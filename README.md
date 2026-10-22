# BattSafe — EV Battery Monitoring + AI-Assisted Anomaly Detection

This starter package contains a Google Colab notebook, sample sensor dataset, Python training script, ESP32 firmware with Blynk IoT integration, and setup instructions.

## Important scope and safety
- This is a prototype for **monitoring and anomaly alerting**, not a certified thermal-runaway predictor.
- The included CSV is **synthetic demonstration data**, not measured EV battery data.
- MQ-2 is a broad gas/smoke sensor and is not a battery-specific off-gas detector.
- Do not connect this prototype directly to an EV battery pack or charger. Use a low-voltage test rig and isolated, correctly rated interfaces.
- The ESP32 firmware uses a lightweight z-score anomaly detector whose baseline statistics are exported by the notebook. It does **not** execute the Python Isolation Forest model on the ESP32. The Isolation Forest is trained/evaluated in Python as a research comparison.
- Thresholds in the sample are placeholders. Collect calibrated, representative data before relying on them.

## Files
- `Battsafe_AI_Training.ipynb` — upload to Google Colab; train/evaluate an Isolation Forest and export baseline statistics.
- `battsafe_sensor_data.csv` — synthetic sample data to verify the workflow.
- `Battsafe_AI_Model.py` — reusable Python training/evaluation/export script.
- `Battsafe_ESP32_Blynk.ino` — ESP32-S3 firmware with Blynk virtual pins, sensor readings, alarms, and demo cutoff flag.
- `Battsafe_model_config.json` — example baseline configuration generated from the sample data.
- `requirements.txt` — Python dependencies.
- `README.md` — these instructions.

## Google Colab steps
1. Open https://colab.research.google.com/
2. Upload `Battsafe_AI_Training.ipynb` and open it.
3. Run the cells from top to bottom. The notebook creates/downloads `Battsafe_model_config.json` and `Battsafe_isolation_forest.joblib`.
4. For your project, replace the synthetic CSV with real, safely collected, calibrated readings using the same column names.

CSV columns:
`temperature_c,battery_voltage_v,gas_adc,condition`
`condition` is for evaluation only (`normal` or `anomaly`); model training uses the normal rows.

## Blynk IoT setup
Create a Blynk template and device. Add these **Datastreams**:
- V0: Temperature (Double, °C)
- V1: Simulated/Measured voltage (Double, V)
- V2: Gas ADC (Integer)
- V3: Anomaly score (Double)
- V4: Alert status (Integer: 0=normal, 1=alert)
- V5: Cutoff request/demo indicator (Integer: 0=inactive, 1=active)

Add widgets to your dashboard and link them to those virtual pins. Copy the Template ID, Template Name, and Device Auth Token from Blynk into the firmware placeholders.

Install Arduino libraries:
- `Blynk`
- `OneWire`
- `DallasTemperature`

Select your ESP32-S3 board and correct port in Arduino IDE. Set Wi-Fi credentials and Blynk credentials before upload.

## ESP32 pin map (verify your specific board)
- DS18B20 DATA: GPIO 4, with 4.7 kΩ pull-up to 3.3 V
- Voltage-simulation potentiometer wiper: GPIO 1 (ADC)
- Gas-simulation potentiometer wiper: GPIO 2 (ADC)
- Buzzer driver: GPIO 5
- Warning LED: GPIO 6 through 220 Ω resistor
- Cutoff indicator LED: GPIO 7 through 220 Ω resistor

Potentiometers are simulation inputs only. The voltage formula assumes a 0–24 V *simulated* range; it is not a substitute for a properly designed, isolated battery measurement circuit.

## How the embedded anomaly score works
The notebook exports normal-data means and standard deviations. The firmware computes z-scores and flags a sample if any feature exceeds its configured z-score limit. This is a small embedded approximation informed by the training data, not the same model as the Python Isolation Forest. Review the generated config and copy its values into the firmware if you retrain the model.

## Safety behavior
The demo cutoff output is an LED/logic signal only. It must not drive an EV charger or battery contactor directly. Any real protective interface needs a qualified electrical design, isolation, correctly rated components, fault analysis, and testing.
