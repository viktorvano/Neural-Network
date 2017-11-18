# Neural-Network
Configutable Neural Network in Cpp, Visual Studio 2017.

This Neural network contains one hidden layer neurons.
The whole topology of neural network can be configured here:

/*************************************************************************************************/
// Free to edit
#define PatternCount 4
#define InputNodes 2
#define HiddenNodes 6
#define OutputNodes 1

const float LearningInputs[PatternCount][InputNodes] = {
	{ 0.0f, 0.0f },
	{ 0.0f, 1.0f },
	{ 1.0f, 0.0f },
	{ 1.0f, 1.0f }
};

const float LearningOutputs[PatternCount][InputNodes] = {// XOR output as an example
	{ 0.0f },
	{ 1.0f },
	{ 1.0f },
	{ 0.0f }
};
/**********************************************************************************************/