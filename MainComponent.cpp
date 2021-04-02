#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent()
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

    // Add keyboard listener
    addKeyListener(this);
}

MainComponent::~MainComponent()
{
    essentia::shutdown();

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

    // Initialise components
    if (analyser == nullptr){
        // Initialise DB connector
        dbConnector = make_unique<DBConnector>();
        analyser = make_unique<Analyser>(*dbConnector);

        // Initialise traverser
        traverser = make_unique<Traverser>(*dbConnector, *analyser);

        samplePanel = make_unique<SamplePanel>(*analyser, *traverser);
        addAndMakeVisible(*samplePanel);

        initialiseGUI();
    }
    analyser->initialise(sampleRate, samplesPerBlockExpected);

    recordingBuffer.setSize(2, GRAINS_IN_TRAJECTORY * GRAIN_LENGTH);
}

void MainComponent::getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill)
{
    int numSamples = bufferToFill.buffer->getNumSamples();
    // Your audio-processing code goes here!
    if(generatedIdx > -1){
        bufferToFill.buffer->clear(0, numSamples);
        bufferToFill.buffer->addFrom(0, 0, generated, 0, generatedIdx, numSamples);
        bufferToFill.buffer->addFrom(1, 0, generated, 1, generatedIdx, numSamples);
        generatedIdx += numSamples;

        if(generatedIdx >= generated.getNumSamples()){
            generatedIdx = -1;
        }
    }

    else if(isRecording){
        // Audio input for recording
        if(recordingIdx + numSamples > recordingBuffer.getNumSamples()){
            isRecording = false;
            recordingIdx = -1;
            triggerAsyncUpdate();

            // Generate trajectory from audio
            generated = traverser->generateTrajectoryFromAudio(recordingBuffer);

            return;
        }
        auto* device = deviceManager.getCurrentAudioDevice();
        auto activeInputChannels  = device->getActiveInputChannels();
        auto activeOutputChannels = device->getActiveOutputChannels();

        recordingBuffer.clear(recordingIdx, numSamples);
        recordingBuffer.addFrom(0, recordingIdx, *bufferToFill.buffer, 0, 0, numSamples);
        recordingBuffer.addFrom(1, recordingIdx, *bufferToFill.buffer, 1, 0, numSamples);

        recordingIdx += numSamples;

        // Disable feedback
        bufferToFill.buffer->clear();
    }

    else if(!isRecording && recordingIdx > -1){
        bufferToFill.buffer->clear(0, numSamples);
        bufferToFill.buffer->addFrom(0, 0, recordingBuffer, 0, recordingIdx, numSamples);
        bufferToFill.buffer->addFrom(1, 0, recordingBuffer, 1, recordingIdx, numSamples);
        recordingIdx += numSamples;

        if(recordingIdx >= recordingBuffer.getNumSamples()){
            recordingIdx = -1;
        }
    }

    else {
        // Disable feedback
        bufferToFill.buffer->clear();
    }
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
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    // You can add your drawing code here!
}

void MainComponent::resized()
{
    // This is called when the MainContentComponent is resized.
    // If you add any child components, this is where you should
    // update their positions.
}

void MainComponent::initialiseGUI(){
    // Add buttons
    // Play button
    playButton = make_unique<TextButton>("playButton");
    playButton->setButtonText("Play");
    playButton->setBounds(24, 224, getWidth() - 24 * 2, 50);
    addAndMakeVisible(*playButton);
    playButton->addListener(this);

    // Create initial random trajectory
    // Only execute if db is populated
    if(dbConnector->isPopulated()){
        generated = traverser->initialiseRandomTrajectory(GRAINS_IN_TRAJECTORY);
    }

    // Yes button
    yesButton = make_unique<TextButton>("yesButton");
    yesButton->setButtonText("Yes");
    yesButton->setBounds(24, 224 + 50 + 24, 100, 50);
    addAndMakeVisible(*yesButton);
    yesButton->addListener(this);

    // No button
    noButton = make_unique<TextButton>("noButton");
    noButton->setButtonText("No");
    noButton->setBounds(100 + 24 + 24, 224 + 50 + 24, 100, 50);
    addAndMakeVisible(*noButton);
    noButton->addListener(this);

    // Reset button
    resetButton = make_unique<TextButton>("resetButton");
    resetButton->setButtonText("Reset");
    resetButton->setBounds(24, 224 + 50 + 50 + 24 + 24, 100, 50);
    addAndMakeVisible(*resetButton);
    resetButton->addListener(this);

    // Add labels
    recordingLabel = make_unique<Label>("recordingLabel");
    recordingLabel->setText("Recording...", dontSendNotification);
    recordingLabel->setBounds(getWidth() - 124, getHeight() - 74, 100, 50);
    addAndMakeVisible(*recordingLabel);
    recordingLabel->setVisible(false);
}

void MainComponent::buttonClicked(juce::Button *button) {
    if(button == playButton.get()){
        // Reset generated buffer index -> plays the current trajectory
        generatedIdx = 0;
    }
    if(button == yesButton.get()){

    }
    if(button == noButton.get()){

    }
    if(button == resetButton.get()){
        // Create new random trajectory
        generated = traverser->initialiseRandomTrajectory(GRAINS_IN_TRAJECTORY);

        // Reset generated buffer index -> plays the current trajectory
        generatedIdx = 0;
    }
}

void MainComponent::oscMessageReceived (const juce::OSCMessage& message){
    String address = message.getAddressPattern().toString();

    if(address == "/path"){
        String content = message[0].getString();
    }
    if(address == "/params"){
        // Handle new set of grain parameters
        vector<float> params;
        for(const auto & element : message){
            params.emplace_back(element.getFloat32());
        }

        // Map list of raw parameters and generate new grain trajectory
        generated = traverser->generateTrajectoryFromParams(params);
        generatedIdx = 0;
    }
}

bool MainComponent::keyPressed(const KeyPress &key, Component *originatingComponent) {
    if(key.getKeyCode() == key.spaceKey){
        // Reset generated buffer index -> plays the current trajectory
        generatedIdx = 0;
    }
    if(key.getTextCharacter() == 'a'){
        // Apply negative
        OSCMessage* message = new OSCMessage("/reward");
        message->addInt32(-1);
        sender.send(*message);
    }
    if(key.getTextCharacter() == 'd'){
        // Apply positive
        OSCMessage* message = new OSCMessage("/reward");
        message->addInt32(1);
        sender.send(*message);
    }
    if(key.getTextCharacter() == 'w'){
        // Apply positive zone feedback
        OSCMessage* message = new OSCMessage("/super_like");
        message->addInt32(1);
        sender.send(*message);
    }
    if(key.getTextCharacter() == 's'){
        // Apply negative zone feedback
        OSCMessage* message = new OSCMessage("/super_like");
        message->addInt32(-1);
        sender.send(*message);
    }
    if(key.getTextCharacter() == 'e'){
        // Change zone
        OSCMessage* message = new OSCMessage("/explore_state");
        message->addInt32(1);
        sender.send(*message);
    }
    if(key.getTextCharacter() == 'r'){
        // Start recording
        isRecording = true;
        recordingLabel->setVisible(isRecording);
        repaint();

        recordingBuffer.clear();
        recordingIdx = 0;
    }
    if(key.getTextCharacter() == 'p'){
        // Start recorded playback
        recordingIdx = 0;
    }
    return true;
}

void MainComponent::showConnectionErrorMessage (const juce::String& messageText)
{
    juce::AlertWindow::showMessageBoxAsync (juce::AlertWindow::WarningIcon,
                                            "Connection error",
                                            messageText,
                                            "OK");
}

void MainComponent::handleAsyncUpdate(){
    recordingLabel->setVisible(isRecording);
    repaint();
}

