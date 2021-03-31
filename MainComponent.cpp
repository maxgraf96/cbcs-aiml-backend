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
                                           [&] (bool granted) { setAudioChannels (granted ? numInputChannels : 0, 2); });
    }
    else
    {
        // Specify the number of input and output channels that we want to open
        setAudioChannels (numInputChannels, 2);
    }

    // Initialise essentia
    essentia::warningLevelActive = false;
    essentia::init();
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
    if (analyser.get() == nullptr){
        // Initialise DB connector
        dbConnector = make_unique<DBConnector>();

        analyser = make_unique<Analyser>(*dbConnector);
        samplePanel = make_unique<SamplePanel>(*analyser.get());
        addAndMakeVisible(*samplePanel);

        // Initialise traverser
        traverser = make_unique<Traverser>(*dbConnector);

        initialiseButtons();
    }
    analyser->initialise(sampleRate, samplesPerBlockExpected);
}

void MainComponent::getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill)
{
    // Your audio-processing code goes here!
    if(generatedIdx > -1){
        bufferToFill.buffer->addFrom(0, 0,generated, 0, generatedIdx, bufferToFill.buffer->getNumSamples());
        bufferToFill.buffer->addFrom(1, 0,generated, 1, generatedIdx, bufferToFill.buffer->getNumSamples());
        generatedIdx += bufferToFill.buffer->getNumSamples();

        if(generatedIdx >= generated.getNumSamples()){
            generatedIdx = -1;
        }
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

void MainComponent::initialiseButtons(){
    // Add buttons
    // Play button
    playButton = make_unique<TextButton>("playButton");
    playButton->setButtonText("Play");
    playButton->setBounds(24, 224, getWidth() - 24 * 2, 50);

    addAndMakeVisible(*playButton);
    playButton->addListener(this);

    // Create initial random trajectory
    generated = traverser->initialiseRandomTrajectory(GRAINS_IN_TRAJECTORY);

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

