﻿// Neural Network.cpp : Defines the entry point for the console application.
//

// Created by: Viktor Vano

#include "stdafx.h"
#include "windows.h"
#include <cmath>
#include <vector>
#include <iostream>
#include <cassert>
#include <sstream>


/*************************************************************************************************/
// Free to edit
#define PatternCount 16
#define InputNodes 4
#define HiddenNodes 32
#define OutputNodes 2
#define velocity 0.1; // overall net learning rate [0.0..1.0]
#define momentum 0.5; // momentum multiplier of last deltaWeight [0.0..n]

const float LearningInputs[PatternCount][InputNodes] = {
	{ 0.0f, 0.0f, 0.0f, 0.0f },
	{ 0.0f, 0.0f, 0.0f, 1.0f },
	{ 0.0f, 0.0f, 1.0f, 0.0f },
	{ 0.0f, 0.0f, 1.0f, 1.0f },
	{ 0.0f, 1.0f, 0.0f, 0.0f },
	{ 0.0f, 1.0f, 0.0f, 1.0f },
	{ 0.0f, 1.0f, 1.0f, 0.0f },
	{ 0.0f, 1.0f, 1.0f, 1.0f },
	{ 1.0f, 0.0f, 0.0f, 0.0f },
	{ 1.0f, 0.0f, 0.0f, 1.0f },
	{ 1.0f, 0.0f, 1.0f, 0.0f },
	{ 1.0f, 0.0f, 1.0f, 1.0f },
	{ 1.0f, 1.0f, 0.0f, 0.0f },
	{ 1.0f, 1.0f, 0.0f, 1.0f },
	{ 1.0f, 1.0f, 1.0f, 0.0f },
	{ 1.0f, 1.0f, 1.0f, 1.0f },
};

const float LearningOutputs[PatternCount][OutputNodes] = {
	{ 0.0f, 1.0f },// LearningOutputs[x][0] represents AND for LearningInputs[x][0,1]
	{ 0.0f, 0.0f },// LearningOutputs[x][1] represents XNOR for LearningInputs[x][0,1,2,3]
	{ 0.0f, 0.0f },
	{ 0.0f, 0.0f },
	{ 0.0f, 0.0f },
	{ 0.0f, 0.0f },
	{ 0.0f, 0.0f },
	{ 0.0f, 0.0f },
	{ 0.0f, 0.0f },
	{ 0.0f, 0.0f },
	{ 0.0f, 0.0f },
	{ 0.0f, 0.0f },
	{ 1.0f, 0.0f },
	{ 1.0f, 0.0f },
	{ 1.0f, 0.0f },
	{ 1.0f, 1.0f },
};
/**********************************************************************************************/

using namespace std;

int training_line = 0;// Has to be initialized 0
std::vector<float> input, target, result;
int trainingPass = 0;

// Training class to read training data from an array
class TrainingData
{
public:	
	unsigned getNextInputs(vector<float> &inputValues);
	unsigned getTargetOutputs(vector<float> &targetOutValues);
};

unsigned TrainingData::getNextInputs(vector<float> &inputValues)
{
	inputValues.clear();

	if (training_line >= PatternCount)
		training_line = 0;

	for (int i = 0; i<InputNodes; i++)
		inputValues.push_back(LearningInputs[training_line][i]);

	return inputValues.size();
}

unsigned TrainingData::getTargetOutputs(vector<float> &targetOutValues)
{
	targetOutValues.clear();

	for (int i = 0; i<OutputNodes; i++)
		targetOutValues.push_back(LearningOutputs[training_line][i]);

	training_line++;

	return targetOutValues.size();
}

struct Connection
{
	float weight;
	float deltaWeight;
};

class Neuron;

typedef vector<Neuron> Layer;

//************ class Neuron **************

class Neuron
{
public:
	Neuron(unsigned numOutputs, unsigned myIndex);
	void setOutputValue(float value) { m_outputValue = value; }
	float getOutputValue(void) const { return m_outputValue; }
	void feedForward(const Layer &prevLayer);
	void calcOutputGradients(float targetValue);
	void calcHiddenGradients(const Layer &nextLayer);
	void updateInputWeights(Layer &prevLayer);

private:
	static float eta; // [0.0..1.0] overall network training rate
	static float alpha; // [0.0..n] multiplier of last weight change (momentum)
	static float transferFunction(float x);
	static float transferFunctionDerivative(float x);
	static float randomWeight(void);
	float sumDOW(const Layer &nextLayer) const;
	float m_outputValue;
	vector<Connection> m_outputWeights;
	unsigned m_myIndex;
	float m_gradient;
};

Neuron::Neuron(unsigned numOutputs, unsigned myIndex)
{
	for (unsigned c = 0; c < numOutputs; c++)
	{
		m_outputWeights.push_back(Connection());
		m_outputWeights.back().weight = randomWeight();
	}

	m_myIndex = myIndex;
}

void Neuron::feedForward(const Layer &prevLayer)
{
	float sum = 0.0;

	// Sum the previous layer's outputs (which are inputs)
	// Include the bias node from the previous layer.

	for (unsigned n = 0; n < prevLayer.size(); n++)
	{
		sum += prevLayer[n].getOutputValue() * prevLayer[n].m_outputWeights[m_myIndex].weight;
	}

	m_outputValue = Neuron::transferFunction(sum);
}

void Neuron::calcOutputGradients(float targetValue)
{
	float delta = targetValue - m_outputValue;
	m_gradient = delta * Neuron::transferFunctionDerivative(m_outputValue);
}

void Neuron::calcHiddenGradients(const Layer &nextLayer)
{
	float dow = sumDOW(nextLayer);
	m_gradient = dow * Neuron::transferFunctionDerivative(m_outputValue);
}

void Neuron::updateInputWeights(Layer &prevLayer)
{
	// The weights to updated are in the Connection container
	// in the neurons in the preceding layer
	for (unsigned n = 0; n < prevLayer.size(); n++)
	{
		Neuron &neuron = prevLayer[n];
		float oldDeltaWeight = neuron.m_outputWeights[m_myIndex].deltaWeight;

		float newDeltaWeight =
			// Individual input, magnified by the gradient and train rate:
			eta // 0.0==slowlearner; 0.2==medium learner; 1.0==reckless learner
			* neuron.getOutputValue()
			* m_gradient
			// Also add momentum = a fraction of the previous delta weight
			+ alpha // 0.0==no momentum; 0.5==moderate momentum
			* oldDeltaWeight;
		neuron.m_outputWeights[m_myIndex].deltaWeight = newDeltaWeight;
		neuron.m_outputWeights[m_myIndex].weight += newDeltaWeight;
	}
}

float Neuron::eta = velocity; // overall net learning rate [0.0..1.0]
float Neuron::alpha = momentum; // momentum multiplier of last deltaWeight [0.0..n]

float Neuron::sumDOW(const Layer &nextLayer) const
{
	float sum = 0.0f;

	// Sum our contributions of the errors at the nodes we feed
	for (unsigned n = 0; n < nextLayer.size() - 1; n++)
	{
		sum += m_outputWeights[n].weight * nextLayer[n].m_gradient;
	}

	return sum;
}

float Neuron::transferFunction(float x)
{
	// tanh - output range [-1.0..1.0]
	return tanh(x);
}

float Neuron::transferFunctionDerivative(float x)
{
	// tanh derivative
	return 1.0 - pow(tanh(x), 2.0);// approximation return 1.0 - x*x;
}

float Neuron::randomWeight(void)
{
	return rand() / float(RAND_MAX);
}

//************ class NeuralNetwork **************

class NeuralNetwork
{
public:
	NeuralNetwork(const vector<unsigned> &topology);
	void feedForward(const vector<float> &inputValues);
	void backProp(const vector<float> &targetValues);
	void getResults(vector<float> &resultValues) const;
	float getRecentAverageError(void) const { return m_recentAverageError; }

private:
	vector<Layer> m_layers; // m_layers[layerNum][neuronNum]
	float m_error;
	float m_recentAverageError;
	float m_recentAverageSmoothingFactor;

};

NeuralNetwork::NeuralNetwork(const vector<unsigned> &topology)
{
	unsigned numLayers = topology.size();
	for (unsigned layerNum = 0; layerNum < numLayers; layerNum++)
	{
		m_layers.push_back(Layer());
		unsigned numOutputs = layerNum == topology.size() - 1 ? 0 : topology[layerNum + 1];

		// We have made a new Layer, now fill it with neurons, and add a bias neuron to the layer.
		for (unsigned neuronNum = 0; neuronNum <= topology[layerNum]; neuronNum++)
		{
			m_layers.back().push_back(Neuron(numOutputs, neuronNum));
			cout << "Made a neuron" << endl;
		}

		// Force the bias node's output value to 1.0. It's last neuron created above
		m_layers.back().back().setOutputValue(1.0);
	}
}

void NeuralNetwork::feedForward(const vector<float> &inputValues)
{
	assert(inputValues.size() == m_layers[0].size() - 1);

	// Assign (latch) the input values into the input neurons
	for (unsigned i = 0; i < inputValues.size(); i++)
	{
		m_layers[0][i].setOutputValue(inputValues[i]);
	}

	// Forward propagate
	for (unsigned layerNum = 1; layerNum < m_layers.size(); layerNum++)
	{
		Layer &prevLayer = m_layers[layerNum - 1];
		for (unsigned n = 0; n < m_layers[layerNum].size() - 1; n++)
		{
			m_layers[layerNum][n].feedForward(prevLayer);
		}
	}
}

void NeuralNetwork::backProp(const vector<float> &targetValues)
{
	// Calculate overall net error (RMS of output neuron errors)
	Layer &outputLayer = m_layers.back();
	m_error = 0.0;

	for (unsigned n = 0; n < outputLayer.size() - 1; n++)
	{
		float delta = targetValues[n] - outputLayer[n].getOutputValue();
		m_error += delta * delta;
	}
	m_error /= outputLayer.size() - 1; //get average errorsquared
	m_error = sqrt(m_error); // RMS

							 // Implement a recent average measurement;

	m_recentAverageError =
		(m_recentAverageError * m_recentAverageSmoothingFactor + m_error)
		/ (m_recentAverageSmoothingFactor + 1.0);

	// Calculate output layer gradients
	for (unsigned n = 0; n < outputLayer.size() - 1; n++)
	{
		outputLayer[n].calcOutputGradients(targetValues[n]);
	}

	// Calculate gradients on hidden layers
	for (unsigned layerNum = m_layers.size() - 2; layerNum > 0; layerNum--)
	{
		Layer &hiddenLayer = m_layers[layerNum];
		Layer &nextLayer = m_layers[layerNum + 1];

		for (unsigned n = 0; n < hiddenLayer.size(); n++)
		{
			hiddenLayer[n].calcHiddenGradients(nextLayer);
		}
	}

	// For all layers from outputs to first hidden layer.
	// update connection weights

	for (unsigned layerNum = m_layers.size() - 1; layerNum > 0; layerNum--)
	{
		Layer &layer = m_layers[layerNum];
		Layer &prevLayer = m_layers[layerNum - 1];

		for (unsigned n = 0; n < layer.size() - 1; n++)
		{
			layer[n].updateInputWeights(prevLayer);
		}
	}
}

void NeuralNetwork::getResults(vector<float> &resultValues) const
{
	resultValues.clear();

	for (unsigned n = 0; n < m_layers.back().size() - 1; n++)
	{
		resultValues.push_back(m_layers.back()[n].getOutputValue());
	}
}

void showVectorValues(string label, vector<float> &v)
{
	cout << label << " ";
	for (unsigned i = 0; i < v.size(); i++)
	{
		cout << v[i] << " ";
	}

	cout << endl;
}

int main()
{
	TrainingData trainData;
	std::vector<unsigned> topology;
	topology = { InputNodes, HiddenNodes, OutputNodes };
	NeuralNetwork myNet(topology);

	cout << endl << "Training started\n" << endl;
	while (1)
	{
		trainingPass++;
		cout << endl << "Pass " << trainingPass;

		//Get new input data and feed it forward:
		trainData.getNextInputs(input);
		showVectorValues(": Inputs:", input);
		myNet.feedForward(input);

		// Train the net what the outputs should have been:
		trainData.getTargetOutputs(target);
		showVectorValues("Targets: ", target);
		assert(target.size() == topology.back());
		myNet.backProp(target);//This function alters neurons

							   // Collect the net's actual results:		
		myNet.getResults(result);
		showVectorValues("Outputs: ", result);


		// Report how well the training is working, averaged over recent samples:
		cout << "Net recent average error: "
			<< myNet.getRecentAverageError() << endl;

		if (myNet.getRecentAverageError() < 0.001)
		{
			cout << "Exit due to low error :D" << endl;
			break;
		}
	}

	cout << endl << "Training done\n" << endl;
	cout << endl << "Run mode begin\n" << endl;
	trainingPass = 0;
	while (1)
	{
		trainingPass++;
		cout << endl << "Run " << trainingPass;

		//Get new input data and feed it forward:
		//Make sure that your input data are the same size as InputNodes
		input = { (float)(rand() % 2), (float)(rand() % 2), (float)(rand() % 2), (float)(rand() % 2) };
		showVectorValues(": Inputs:", input);
		myNet.feedForward(input);

		// Collect the net's actual results:
		myNet.getResults(result);
		showVectorValues("Outputs: ", result);

		Sleep(1000);
	}

	system("pause");

	return 0;
}

// Created by: Viktor Vano
