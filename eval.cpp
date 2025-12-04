#include <Arduino.h>
#include "esn.h"
#include "imu_features.h"
//#include "datautils.h" // assume that the data processor is defined here
#include "io.h"

uint16_t CONFUSION_MATRIX[3][3] = {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}};
FeatureVector testWindow[WINDOW_SIZE];
uint8_t testLabels[WINDOW_SIZE];
uint16_t n_samples = 0;
uint16_t correct = 0;

void resetMetrics() {
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) 
            CONFUSION_MATRIX[i][j] = 0;
    }
}

void printMultiClassMetrics(uint16_t n_samples) {
    const float EPSILON = 0.000001f;
    float precision_sum = 0.0f;
    float recall_sum = 0.0f;

    Coms.send("\n--- Confusion Matrix ---\n");

    for (int i = 0; i < 3; i++) {

        // True Positive (TP_i) = C[i][i]
        uint16_t TP_i = CONFUSION_MATRIX[i][i];
        
        // False Positive 
        uint16_t FP_i = 0;
        for (int k = 0; k < 3; k++) {
            if (k != i)
                FP_i += CONFUSION_MATRIX[k][i];
        }
        
        // False Negative
        uint16_t FN_i = 0;
        for (int k = 0; k < 3; k++) {
            if (k != i)
                FN_i += CONFUSION_MATRIX[i][k];
        }

        // Precision_i 
        float denominator_p = (float)TP_i + FP_i;
        float precision_i = 0.0f;
        if (denominator_p > EPSILON) {
            precision_i = (float)TP_i / denominator_p;
        }
        precision_sum += precision_i;

        // Recall_i
        float denominator_r = (float)TP_i + FN_i;
        float recall_i = 0.0f;
        if (denominator_r > EPSILON) {
            recall_i = (float)TP_i / denominator_r;
        }
        recall_sum += recall_i; 

        // print confusion matrix row
        Coms.send("Actual Class: "); 
        Coms.send(String(i)); 
        Coms.send("\n");

        Coms.send("  Predicted 0: ");
        Coms.send(String(CONFUSION_MATRIX[i][0])); 
        Coms.send("\n");

        Coms.send("  Predicted 1: ");
        Coms.send(String(CONFUSION_MATRIX[i][1])); 
        Coms.send("\n");

        Coms.send("  Predicted 2: ");
        Coms.send(String(CONFUSION_MATRIX[i][2])); 
        Coms.send("\n");
                
    }

    // compute macro-averaged precision, recall, F1-score
    float macro_precision = precision_sum / 3.0f;
    float macro_recall = recall_sum / 3.0f;
    
    float denominator_f1 = macro_precision + macro_recall;
    float macro_f1_score = 0.0f;
    if (denominator_f1 > EPSILON) {
        macro_f1_score = 2.0f * (macro_precision * macro_recall) / denominator_f1;
    }

    // print macro-averaged metrics
    Coms.send("Macro Precision: ");
    Coms.send(String(macro_precision * 100.0f));
    Coms.send(" %\n");

    Coms.send("Macro Recall:    ");
    Coms.send(String(macro_recall * 100.0f));
    Coms.send(" %\n");

    Coms.send("Macro F1-Score:  ");
    Coms.send(String(macro_f1_score * 100.0f));
    Coms.send(" %\n");
}

void evaluateModel(const FeatureVector* testWindow, const uint8_t* testLabels, \
    uint16_t n_samples) {
    

    for (int i = 0; i < n_samples; i++) {
        updateReservoir(testWindow[i]);
        uint8_t prediction = predict();
        if (prediction == testLabels[i]) {
            correct++;
        }
        if (testLabels[i] < 3 && prediction < 3)
            CONFUSION_MATRIX[testLabels[i]][prediction]++;
    }


}

void evaluateLoop(bool printMetrics) {

    resetMetrics();
    uint16_t sum = 0;
    correct = 0;
	n_samples = 0;

    for (int i = 0; i < 3; i++) {
        delay(1000);
        Coms.send("\n--- Evaluation Loop ---");
        Coms.send(String(i + 1));
        Coms.send(" ------------------------\n");
        Coms.send("Collect evaluation data");
        delay(500);
        Coms.send("Start collecting now!\n");
        collectWindow(testWindow, &n_samples);
		Coms.send("Data collected. Waiting for Label input...");
        delay(1500);
        sum += n_samples;
        if (!Coms.getLabel(testLabels, n_samples)) {
				Coms.send("Bad input");
			    break;
			}
        normalizeWindow(testWindow, n_samples);
        evaluateModel(testWindow, testLabels, n_samples);
        delay(500);
    }
    
    float accuracy = (float)correct / sum * 100.0f;
    Coms.send("Evaluation Accuracy: ");
    Coms.send(String(accuracy));
    Coms.send("%");

    // default as true: For precision, recall, F1-score and confusion matrix
    if (printMetrics) 
        printMultiClassMetrics(sum);

}