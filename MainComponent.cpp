#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent()
: audioSetupComp (deviceManager,
 0,     // minimum input channels
 256,   // maximum input channels
 0,     // minimum output channels
 256,   // maximum output channels
 false, // ability to select midi inputs
 false, // ability to select midi output device
 false, // treat channels as stereo pairs
 false) // hide advanced options
{
    // Make sure you set the size of the component after
    // you add any child components.
    setSize (1024, 600);

    // Disable input
    int numInputChannels = 0;

    // Some platforms require permissions to open input channels so request that here
    if (juce::RuntimePermissions::isRequired (juce::RuntimePermissions::recordAudio)
        && ! juce::RuntimePermissions::isGranted (juce::RuntimePermissions::recordAudio))
    {
        juce::RuntimePermissions::request (juce::RuntimePermissions::recordAudio,
                                           [&] (bool granted) { setAudioChannels (granted ? numInputChannels : 2, 2); });
    }
    else
    {
        // Specify the number of input and output channels that we want to open
        setAudioChannels (2, 2);
    }

    // Initialise essentia
    essentia::warningLevelActive = false;
    essentia::init();

    // Initialise OSC
    // Server
    if (!connect (oscListeningPort))
        showConnectionErrorMessage ("Error: could not connect to UDP port " + to_string(oscListeningPort));

    // Sender
    if (!sender.connect ("127.0.0.1", oscSenderPort))
        showConnectionErrorMessage ("Error: could not connect to UDP port " + to_string(oscSenderPort));

    // Add OSC path listeners
    addListener (this, "/path");
    addListener (this, "/params");
    addListener (this, "/osc_from_js");
    addListener (this, "/osc_from_js_is_looping");
    addListener (this, "/osc_from_js_left_channel_data");
    addListener (this, "/osc_from_js_right_channel_data");
    addListener (this, "/osc_from_js_audio_transmission_done");
    addListener (this, "/osc_from_js_clear_recording_buffer");
    addListener (this, "/osc_from_js_play");
    addListener (this, "/osc_from_js_agent_feedback");
    addListener (this, "/osc_from_js_agent_zone_feedback");
    addListener (this, "/osc_from_js_pause_agent");
    addListener (this, "/osc_from_js_record_in_JUCE");
    addListener (this, "/osc_from_js_explore");
    addListener (this, "/explore_state_done");

    // Add keyboard listener
    addKeyListener(this);

    setupDiagnosticsAndDeviceManager();
    LookAndFeel::getDefaultLookAndFeel().setDefaultSansSerifTypefaceName ("Arial");
}

MainComponent::~MainComponent()
{
    essentia::shutdown();
    sender.disconnect();

    deviceManager.removeChangeListener (this);
    // This shuts down the audio device and clears the audio source.
    shutdownAudio();
}

//==============================================================================
void MainComponent::prepareToPlay (int samplesPerBlockExpected, double sampleRate)
{
    // This function will be called when the audio device is started, or when
    // its settings (i.e. sample rate, block size, etc) are changed.

    // You can use this function to initialise any resources you might need,
    // but be careful - it will be called on the audio thread, not the GUI thread.

    // For more details, see the help for AudioProcessor::prepareToPlay()

    // Reinitialise essentia if it is not initialised
    if(!essentia::isInitialized()){
        essentia::warningLevelActive = false;
        essentia::init();
    }

    if (generatedBuffer == nullptr){
        generatedBuffer = make_unique<AudioBuffer<float>>(2, GRAIN_LENGTH * GRAINS_IN_TRAJECTORY);
        generatedBuffer->clear();
    }
    generatedIdx = -1;
    recordingIdx = -1;
    canSetGeneratedIdx = true;

    if(recordingBuffer == nullptr)
    {
        recordingBuffer = make_unique<AudioBuffer<float>>(2, GRAINS_IN_TRAJECTORY * GRAIN_LENGTH);
        recordingBuffer->clear();
    }

    // Initialise components
    if (analyser == nullptr){
        // Initialise DB connector
        dbConnector = make_unique<DBConnector>();
        analyser = make_unique<Analyser>(*dbConnector);

        // Initialise traverser
        traverser = make_unique<Traverser>(*dbConnector, *analyser, *generatedBuffer, generatedGrains);

        samplePanel = make_unique<SamplePanel>(*analyser, *traverser);
        addAndMakeVisible(*samplePanel);

        initialiseGUI();
    }
    analyser->initialise(sampleRate, samplesPerBlockExpected);
}

void MainComponent::getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill)
{
    canSetGeneratedIdx = false;
    int numSamples = bufferToFill.buffer->getNumSamples();
    int genIdx = generatedIdx;
    // Normal, linear, one-time playback of trajectory
    if(!isLooping && !isGeneratedLooping && isAgentPaused && genIdx > -1){
        if(genIdx + numSamples > generatedBuffer->getNumSamples()){
            generatedIdx = genIdx = -1;
            triggerAsyncUpdate();
            return;
        }
        bufferToFill.buffer->clear(0, numSamples);
        bufferToFill.buffer->addFrom(0, 0, *generatedBuffer, 0, genIdx, numSamples);
        bufferToFill.buffer->addFrom(1, 0, *generatedBuffer, 1, genIdx, numSamples);
        generatedIdx += numSamples;
        return;
    }

    else if(isRecording){
        // Audio input for recording
        if(recordingIdx + numSamples > recordingBuffer->getNumSamples()){
            isRecording = false;
            recordingIdx = -1;
            shouldPrimeTrajectory = true;
            triggerAsyncUpdate();
            return;
        }
        auto* device = deviceManager.getCurrentAudioDevice();
        auto activeInputChannels  = device->getActiveInputChannels();
        auto activeOutputChannels = device->getActiveOutputChannels();

        recordingBuffer->clear(recordingIdx, numSamples);
        recordingBuffer->addFrom(0, recordingIdx, *bufferToFill.buffer, 0, 0, numSamples);
        recordingBuffer->addFrom(1, recordingIdx, *bufferToFill.buffer, 1, 0, numSamples);

        recordingIdx += numSamples;

        // Disable feedback
        bufferToFill.buffer->clear();
        return;
    }

    else if(!isRecording && recordingIdx > -1){
        if(recordingIdx + numSamples >= recordingBuffer->getNumSamples()){
            recordingIdx = -1;
            return;
        }
        bufferToFill.buffer->clear(0, numSamples);
        bufferToFill.buffer->addFrom(0, 0, *recordingBuffer, 0, recordingIdx, numSamples);
        bufferToFill.buffer->addFrom(1, 0, *recordingBuffer, 1, recordingIdx, numSamples);
        recordingIdx += numSamples;

        return;
    }

    else if(isLooping){
        if(genIdx >= loopingGrainEndIdx){
            generatedIdx = genIdx = loopingGrainStartIdx;
        }
        bufferToFill.buffer->clear(0, numSamples);
        bufferToFill.buffer->addFrom(0, 0, *generatedBuffer, 0, genIdx, numSamples);
        bufferToFill.buffer->addFrom(1, 0, *generatedBuffer, 1, genIdx, numSamples);
        generatedIdx += numSamples;
        return;
    } else if (generatedIdx > -1 && (!isAgentPaused || isGeneratedLooping)){
        // Loop generated
        if(genIdx + numSamples > generatedBuffer->getNumSamples()){
            generatedIdx = genIdx = 0;
        }
        bufferToFill.buffer->clear(0, numSamples);
        bufferToFill.buffer->addFrom(0, 0, *generatedBuffer, 0, genIdx, numSamples);
        bufferToFill.buffer->addFrom(1, 0, *generatedBuffer, 1, genIdx, numSamples);
        generatedIdx += numSamples;
        return;
    }

    else {
        // Disable feedback
        bufferToFill.buffer->clear();
    }
    canSetGeneratedIdx = true;
}

void MainComponent::releaseResources()
{
    // This will be called when the audio device stops, or when it is being
    // restarted due to a setting change.

    // For more details, see the help for AudioProcessor::releaseResources()
}

//==============================================================================
void MainComponent::paint (juce::Graphics& g)
{
    // Black background
    g.fillAll (Colour::fromRGB(0, 0, 0));
}

void MainComponent::resized()
{
    // This is called when the MainContentComponent is resized.
    // If you add any child components, this is where you should
    // update their positions.

    auto rect = getLocalBounds();
    audioSetupComp.setBounds (rect.removeFromLeft (proportionOfWidth (0.6f)));
    rect.reduce (10, 10);

    auto topLine (rect.removeFromTop (20));
    rect.removeFromTop (20);

    diagnosticsBox.setBounds (rect);
}

void MainComponent::initialiseGUI(){
    // Add buttons
    int marginTop = samplePanel->getHeight();
    int buttonHeight = 80;

    // Record button
    recordButton = make_unique<TextButton>("recordButton");
    recordButton->setButtonText("Record");
    recordButton->setBounds(0, marginTop, getWidth() / 2, buttonHeight);
    recordButton->addListener(this);
    recordButton->setLookAndFeel(&myLookAndFeel);
    recordButton->setColour(TextButton::buttonColourId, recordButtonBackground);
    addAndMakeVisible(*recordButton);

    // Play button
    playButton = make_unique<TextButton>("playButton");
    playButton->setButtonText("Play");
    playButton->setBounds(getWidth() / 2, marginTop, getWidth() / 2, buttonHeight);
    addAndMakeVisible(*playButton);
    playButton->setLookAndFeel(&myLookAndFeel);
    playButton->setColour(TextButton::buttonColourId, playButtonBackground);
    playButton->addListener(this);

    // Create initial random trajectory
    // Only execute if db is populated
    if(dbConnector->isPopulated()){
        traverser->generateRandomTrajectory();
    }

    // No button
    noButton = make_unique<TextButton>("noButton");
    noButton->setButtonText("-");
    noButton->setBounds(0, marginTop + buttonHeight, getWidth() / 2, buttonHeight);
    noButton->addListener(this);
    noButton->setLookAndFeel(&myLookAndFeel);
    noButton->setColour(TextButton::buttonColourId, feedbackButtonBackground);
    addAndMakeVisible(*noButton);

    // Yes button
    yesButton = make_unique<TextButton>("yesButton");
    yesButton->setButtonText("+");
    yesButton->setBounds(getWidth() / 2, marginTop + buttonHeight, getWidth() / 2, buttonHeight);
    yesButton->addListener(this);
    yesButton->setLookAndFeel(&myLookAndFeel);
    yesButton->setColour(TextButton::buttonColourId, feedbackButtonBackground);
    addAndMakeVisible(*yesButton);

    // Superdislike button
    superdislikeButton = make_unique<TextButton>("superdislikeButton");
    superdislikeButton->setButtonText(CharPointer_UTF8 ("\xe2\x96\xbc"));
    superdislikeButton->setBounds(0, marginTop + buttonHeight * 2, getWidth() / 2, buttonHeight);
    superdislikeButton->addListener(this);
    superdislikeButton->setLookAndFeel(&myLookAndFeel);
    superdislikeButton->setColour(TextButton::buttonColourId, feedbackButtonBackground);
    addAndMakeVisible(*superdislikeButton);

    superlikeButton = make_unique<TextButton>("superlikeButton");
    superlikeButton->setButtonText(CharPointer_UTF8 ("\xe2\x96\xb2"));
    superlikeButton->setBounds(getWidth() / 2, marginTop + buttonHeight * 2, getWidth() / 2, buttonHeight);
    superlikeButton->addListener(this);
    superlikeButton->setLookAndFeel(&myLookAndFeel);
    superlikeButton->setColour(TextButton::buttonColourId, feedbackButtonBackground);
    addAndMakeVisible(*superlikeButton);

    // Run agent button
    runAgentButton = make_unique<TextButton>("runAgentButton");
    runAgentButton->setButtonText("Run agent");
    runAgentButton->setBounds(0, marginTop + buttonHeight * 3, getWidth(), buttonHeight);
    runAgentButton->addListener(this);
    runAgentButton->setLookAndFeel(&myLookAndFeel);
    runAgentButton->setColour(TextButton::buttonColourId, black);
    addAndMakeVisible(*runAgentButton);

    // Explore button
    exploreButton = make_unique<TextButton>("exploreButton");
    exploreButton->setButtonText("Explore");
    exploreButton->setBounds(0, marginTop + buttonHeight * 4, getWidth(), buttonHeight);
    exploreButton->addListener(this);
    exploreButton->setLookAndFeel(&myLookAndFeel);
    exploreButton->setColour(TextButton::buttonColourId, black);
    addAndMakeVisible(*exploreButton);

    // Reset button
    resetButton = make_unique<TextButton>("resetButton");
    resetButton->setButtonText("Reset");
    resetButton->setBounds(0, getHeight() - buttonHeight, getWidth() / 4, buttonHeight);
    resetButton->addListener(this);
    resetButton->setLookAndFeel(&myLookAndFeel);
    resetButton->setColour(TextButton::buttonColourId, feedbackButtonBackground);
    addAndMakeVisible(*resetButton);
}

void MainComponent::buttonClicked(juce::Button *button) {
    if(button == recordButton.get()){
        isRecording = true;
        recordButton->setButtonText("Recording...");
        repaint();
        recordingBuffer->clear();
        recordingIdx = 0;
    }
    if(button == playButton.get()){
        // Reset generated buffer index -> plays the current trajectory
        if(generatedIdx > 0){
            generatedIdx = -1;
            playButton->setButtonText("Play");
        }
        else {
            generatedIdx = 0;
            playButton->setButtonText("Stop");
        }

        if (isGeneratedLooping){
            playButton->setButtonText("Play");
            isGeneratedLooping = false;
            generatedIdx = -1;
        }
    }
    if(button == noButton.get()){
        // Apply feedback
        OSCMessage msgOut = OSCMessage("/reward");
        msgOut.addInt32(-1);
        sender.send(msgOut);
    }
    if(button == yesButton.get()){
        OSCMessage msgOut = OSCMessage("/reward");
        msgOut.addInt32(1);
        sender.send(msgOut);
    }
    if(button == superdislikeButton.get()){
        OSCMessage msgOut = OSCMessage("/super_like");
        msgOut.addInt32(-1);
        sender.send(msgOut);
    }
    if(button == superlikeButton.get()){
        OSCMessage msgOut = OSCMessage("/super_like");
        msgOut.addInt32(1);
        sender.send(msgOut);
    }
    if(button == runAgentButton.get()){
        isGeneratedLooping = false;
        isAgentPaused = !isAgentPaused;
        if(isAgentPaused){
            runAgentButton->setColour(TextButton::buttonColourId, black);
            runAgentButton->setButtonText("Run agent");
            generatedIdx = -1;
        } else {
            runAgentButton->setColour(TextButton::buttonColourId, playButtonBackground);
            runAgentButton->setButtonText("Agent running...");
            while(!canSetGeneratedIdx){
                std::this_thread::sleep_for(10ms);
            }
            generatedIdx = 0;
        }
        repaint();

        // Pause/unpause RL agent
        OSCMessage msgOut = OSCMessage("/pause");
        msgOut.addInt32(isAgentPaused);
        sender.send(msgOut);
    }
    if(button == exploreButton.get()){
        OSCMessage msgOut = OSCMessage("/explore_state");
        msgOut.addInt32(1);
        sender.send(msgOut);

        exploreButton->setButtonText("Exploring...");
        repaint();
    }
    if(button == resetButton.get()){
        OSCMessage message = OSCMessage("/restart_script");
        sender.send(message);

        // Stop playing
        generatedIdx = -1;
    }
}

void MainComponent::oscMessageReceived (const juce::OSCMessage& message){
    String address = message.getAddressPattern().toString();

    if(address == "/params"){
        // Handle new set of grain parameters
        vector<float> params;
        for(const auto & element : message){
            params.emplace_back(element.getFloat32());
        }
        // Generate new grain trajectory from RL parameters
        traverser->generateTrajectoryFromParams(params);

        fprintf(stdout, "New trajectory from RL params created.\n");
        // Notify RL agent that we are ready to receive the next trajectory
        OSCMessage msgOut = OSCMessage("/juce_ready_for_next");
        sender.send(msgOut);
    }

    if(address == "/osc_from_js"){
        // Handle incoming OSC data from mobile JS interface:
        // This is for playback of an existing trajectory. We want to loop a grain based on the
        // phone orientation. Through tilting the phone users can "go along" a trajectory.
        // First, map the incoming value (in the range of [0...1]) to nearest grain start index
        float incoming = message[0].getFloat32();
        loopingGrainStartIdx = static_cast<int>(mapFloat(incoming, 0.0f, 1.0f, 0.0f, static_cast<float>(GRAINS_IN_TRAJECTORY - 1))) * GRAIN_LENGTH;
        loopingGrainEndIdx = loopingGrainStartIdx + GRAIN_LENGTH;
    }
    if(address == "/osc_from_js_is_looping"){
        isLooping = message[0].getInt32() != 0;
        if(isLooping){
            generatedIdx = loopingGrainStartIdx;
        } else {
            generatedIdx = -1;
        }
    }
    if(address == "/osc_from_js_clear_recording_buffer"){
        recordingBuffer->clear();
        oscCounter = 0;
    }
    int messageSize = 100;
    if(address == "/osc_from_js_left_channel_data"){
        oscCounter++;
        int bufferIdx = message[0].getInt32();
        recordingBuffer->clear(0, bufferIdx, messageSize);
        for(int i = 1; i < message.size(); i++){
            recordingBuffer->addSample(0, bufferIdx++, message[i].getFloat32());
        }
    }
    if(address == "/osc_from_js_right_channel_data"){
        oscCounter++;
        int bufferIdx = message[0].getInt32();
        recordingBuffer->clear(1, bufferIdx, messageSize);
        for(int i = 1; i < message.size(); i++){
            recordingBuffer->addSample(1, bufferIdx++, message[i].getFloat32());
        }
    }
    if(address == "/osc_from_js_audio_transmission_done"){
        fprintf(stdout, "OSC done, floats received: %i / in percent: %f", oscCounter * messageSize, (static_cast<float>(oscCounter * messageSize) / (2.0f * 74496.0f)));
        fprintf(stdout, "\n");
        // Change button text back

        repaint();

        // Normalise buffer
//        float newMaximum = 0.99f;
//        int numSamples = recordingBuffer.getNumSamples() - 1;
//        float maxL = findMaximum(recordingBuffer.getReadPointer(0, numSamples), numSamples);
//        float normaliseFactor = newMaximum / maxL;
//        recordingBuffer.applyGain(normaliseFactor);

        // Generate new grain trajectory from recorded audio
        traverser->generateTrajectoryFromAudio(*recordingBuffer);
        primeTrajectory();
    }
    if(address == "/osc_from_js_play"){
        playButton->triggerClick();
    }
    if(address == "/osc_from_js_agent_feedback"){
        // -1 for negative, 1 for positive
        int feedback = message[0].getInt32();
        if(feedback == -1){
            noButton->triggerClick();
        } else {
            yesButton->triggerClick();
        }
    }
    if(address == "/osc_from_js_agent_zone_feedback"){
        // Apply zone feedback
        int feedback = message[0].getInt32();
        if(feedback == -1){
            superdislikeButton->triggerClick();
        } else {
            superlikeButton->triggerClick();
        }
    }
    if(address == "/osc_from_js_pause_agent"){
        runAgentButton->triggerClick();
    }
    if(address == "/osc_from_js_record_in_JUCE"){
        // Start recording
        recordButton->triggerClick();
    }
    if(address == "/osc_from_js_explore"){
        // Change zone
        exploreButton->triggerClick();
    }
    if(address == "/explore_state_done"){
        exploreButton->setButtonText("Explore");
        repaint();
    }
}

bool MainComponent::keyPressed(const KeyPress &key, Component *originatingComponent) {
    if(key.getKeyCode() == KeyPress::spaceKey){
        playButton->triggerClick();
    }
    if(key.getTextCharacter() == 'a'){
        // Apply negative
        noButton->triggerClick();
    }
    if(key.getTextCharacter() == 'd'){
        // Apply positive
        yesButton->triggerClick();
    }
    if(key.getTextCharacter() == 'w'){
        // Apply positive zone feedback
        superlikeButton->triggerClick();
    }
    if(key.getTextCharacter() == 's'){
        // Apply negative zone feedback
        superdislikeButton->triggerClick();
    }
    if(key.getTextCharacter() == 'e'){
        exploreButton->triggerClick();
    }
    if(key.getTextCharacter() == 'r'){
        // Start recording
        recordButton->triggerClick();
    }
    if(key.getTextCharacter() == 'p'){
        // Start recorded playback
        recordingIdx = 0;
    }
    if(key.getTextCharacter() == 't'){
        primeTrajectory();
    }
    if(key.getTextCharacter() == '.'){
        runAgentButton->triggerClick();
    }
    if(key.getTextCharacter() == 'l'){
        // Loop generated
        isGeneratedLooping = !isGeneratedLooping;
        if(isGeneratedLooping){
            generatedIdx = 0;
            playButton->setButtonText("Looping...");
        } else {
            generatedIdx = -1;
            playButton->setButtonText("Play");
        }
    }
    if(key.getKeyCode() == KeyPress::backspaceKey){
        resetButton->triggerClick();
    }
    if(key.getTextCharacter() == 'h'){
        isManagerVisible = !isManagerVisible;
        audioSetupComp.setVisible(isManagerVisible);
        diagnosticsBox.setVisible(isManagerVisible);
    }
    repaint();
    return true;
}

void MainComponent::showConnectionErrorMessage (const juce::String& messageText)
{
    juce::AlertWindow::showMessageBoxAsync (juce::AlertWindow::WarningIcon,"Connection error", messageText,"OK");
}

void MainComponent::handleAsyncUpdate(){
    if(isRecording){
        recordButton->setButtonText("Recording...");
    } else {
        recordButton->setButtonText("Record");
    }
    if(generatedIdx == -1 && !isGeneratedLooping){
        playButton->setButtonText("Play");
    } else if(isGeneratedLooping){
        playButton->setButtonText("Looping...");
    } else {
        playButton->setButtonText("Stop");
    }
    repaint();

    if(shouldPrimeTrajectory){
        shouldPrimeTrajectory = false;
        // Generate trajectory from audio
        traverser->generateTrajectoryFromAudio(*recordingBuffer);
        // Send to RL
        primeTrajectory();
    }
}

void MainComponent::primeTrajectory() {
    // Create grains from recording and send to RL agent
    OSCMessage message = OSCMessage("/prime_trajectory");

    // Convert recording buffer to grains
    vector<float> data = traverser->convertAudioBufferToRLFormat(*recordingBuffer);

    // Each attribute of each grain is one message argument
    for(auto& entry : data){
        message.addFloat32(entry);
    }

    sender.send(message);
}

void MainComponent::setupDiagnosticsAndDeviceManager(){
    addAndMakeVisible (audioSetupComp);
    addAndMakeVisible (diagnosticsBox);

    audioSetupComp.setVisible(false);
    diagnosticsBox.setVisible(false);

    diagnosticsBox.setMultiLine (true);
    diagnosticsBox.setReturnKeyStartsNewLine (true);
    diagnosticsBox.setReadOnly (true);
    diagnosticsBox.setScrollbarsShown (true);
    diagnosticsBox.setCaretVisible (false);
    diagnosticsBox.setPopupMenuEnabled (true);
    diagnosticsBox.setColour (juce::TextEditor::backgroundColourId, juce::Colour (0x32ffffff));
    diagnosticsBox.setColour (juce::TextEditor::outlineColourId,    juce::Colour (0x1c000000));
    diagnosticsBox.setColour (juce::TextEditor::shadowColourId,     juce::Colour (0x16000000));

    setAudioChannels (2, 2);
    deviceManager.addChangeListener (this);
}

void MainComponent::dumpDeviceInfo()
{
    logMessage ("--------------------------------------");
    logMessage ("Current audio device type: " + (deviceManager.getCurrentDeviceTypeObject() != nullptr
                                                 ? deviceManager.getCurrentDeviceTypeObject()->getTypeName()
                                                 : "<none>"));

    if (auto* device = deviceManager.getCurrentAudioDevice())
    {
        logMessage ("Current audio device: "   + device->getName().quoted());
        logMessage ("Sample rate: "    + juce::String (device->getCurrentSampleRate()) + " Hz");
        logMessage ("Block size: "     + juce::String (device->getCurrentBufferSizeSamples()) + " samples");
        logMessage ("Bit depth: "      + juce::String (device->getCurrentBitDepth()));
        logMessage ("Input channel names: "    + device->getInputChannelNames().joinIntoString (", "));
        logMessage ("Output channel names: "   + device->getOutputChannelNames().joinIntoString (", "));
    }
    else
    {
        logMessage ("No audio device open");
    }
}

void MainComponent::logMessage (const juce::String& m)
{
    diagnosticsBox.moveCaretToEnd();
    diagnosticsBox.insertTextAtCaret (m + juce::newLine);
}

void MainComponent::changeListenerCallback (juce::ChangeBroadcaster*){
    dumpDeviceInfo();
}
