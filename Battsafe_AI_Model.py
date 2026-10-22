"""
Battsafe_AI_Model.py
Train/evaluate an Isolation Forest using a CSV with:
temperature_c,battery_voltage_v,gas_adc,condition
The `condition` column is used for evaluation only. Train on normal rows.
"""
from pathlib import Path
import json
import joblib
import numpy as np
import pandas as pd
from sklearn.ensemble import IsolationForest
from sklearn.metrics import classification_report, confusion_matrix
from sklearn.model_selection import train_test_split

FEATURES = ["temperature_c", "battery_voltage_v", "gas_adc"]

def main(csv_path="battsafe_sensor_data.csv"):
    df = pd.read_csv(csv_path)
    missing = set(FEATURES + ["condition"]) - set(df.columns)
    if missing:
        raise ValueError(f"CSV is missing columns: {sorted(missing)}")

    df = df.dropna(subset=FEATURES + ["condition"])
    normal = df[df["condition"].astype(str).str.lower() == "normal"]
    if len(normal) < 20:
        raise ValueError("Need at least 20 normal samples to fit a useful baseline.")

    X_normal = normal[FEATURES].astype(float)
    model = IsolationForest(
        n_estimators=200,
        contamination=0.05,
        random_state=42
    )
    model.fit(X_normal)

    X_all = df[FEATURES].astype(float)
    y_true = (df["condition"].astype(str).str.lower() != "normal").astype(int)
    y_pred = (model.predict(X_all) == -1).astype(int)
    print("Evaluation on supplied rows (demonstration only):")
    print(classification_report(y_true, y_pred, target_names=["normal", "anomaly"], zero_division=0))
    print("Confusion matrix [normal, anomaly]:")
    print(confusion_matrix(y_true, y_pred))

    joblib.dump(model, "Battsafe_isolation_forest.joblib")

    # Export a lightweight normal-baseline approximation for ESP32.
    # Use only normal rows; standard deviations are floored to avoid division by zero.
    means = X_normal.mean()
    stds = X_normal.std(ddof=0).replace(0, 1e-6)
    config = {
        "model_name": "Battsafe embedded z-score baseline",
        "features": FEATURES,
        "means": {k: float(means[k]) for k in FEATURES},
        "stds": {k: float(stds[k]) for k in FEATURES},
        "z_limit": 3.0,
        "warning_note": "Demonstration only. Calibrate with real sensor data; not a safety-certified predictor."
    }
    Path("Battsafe_model_config.json").write_text(json.dumps(config, indent=2))
    print("\nSaved Battsafe_isolation_forest.joblib and Battsafe_model_config.json")
    print("Copy means/stds/z_limit from Battsafe_model_config.json into ESP32 firmware.")
    print("The ESP32 uses a lightweight z-score approximation, not the Python Isolation Forest itself.")

if __name__ == "__main__":
    main()
