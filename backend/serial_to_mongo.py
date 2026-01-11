import argparse
import time
import json
import sys
from pymongo import MongoClient
import serial
import joblib
import pandas as pd
import warnings

# ---------- Config ----------
MONGO_URI = "mongodb://localhost:27017/"
DB_NAME = "farm_iot"
COLLECTION_NAME = "sensor_readings"
MODEL_PATH = "model/pest_rf_model.pkl"
LABEL_ENCODER_PATH = "model/label_encoder.pkl"
# ----------------------------


def load_model():
    model = joblib.load(MODEL_PATH)
    label_enc = joblib.load(LABEL_ENCODER_PATH)
    print(f"[INFO] Loaded model from {MODEL_PATH}")
    print(f"[INFO] Loaded label encoder from {LABEL_ENCODER_PATH}")
    return model, label_enc


def node_to_crop_encoded(node_name: str):
    # Carrot -> 0, Tomato -> 1
    if not isinstance(node_name, str):
        return 0
    return 1 if node_name.strip().lower() == "tomato" else 0


def parse_line(line: str):
    line = line.strip()
    if not line:
        return None

    if line.startswith("Received:"):
        line = line[len("Received:"):].strip()

    if not line.startswith("{"):
        return None

    try:
        return json.loads(line)
    except Exception as e:
        print(f"[WARN] JSON parse failed: {e}")
        return None


def build_feature_vector(obj):
    mq = obj.get("gas") or obj.get("mq135") or 0.0
    temp = obj.get("temp") or 0.0
    hum = obj.get("hum") or 0.0
    soil = obj.get("soil") or 0.0
    node = obj.get("node") or obj.get("crop")

    return [
        float(mq),
        float(temp),
        float(hum),
        float(soil),
        node_to_crop_encoded(node)
    ]


def enrich_and_insert(mongo_coll, model, label_enc, obj):
    # ---------- ML prediction ----------
    try:
        fv = build_feature_vector(obj)
        feature_names = ["mq135", "temp", "hum", "soil", "crop_encoded"]
        X = pd.DataFrame([fv], columns=feature_names)
        with warnings.catch_warnings():
            warnings.simplefilter("ignore")
            pred_idx = int(model.predict(X)[0])
            
        pred_label = label_enc.inverse_transform([pred_idx])[0]

        try:
            pred_prob = float(max(model.predict_proba(X)[0]))
        except Exception:
            pred_prob = None

    except Exception as e:
        print("[ERROR] ML prediction failed:", e)
        pred_label = None
        pred_prob = None

    # ---------- ML-based risk (authoritative) ----------
    if pred_label == "No Pest":
        ml_risk = 0.05
    elif pred_prob is not None:
        ml_risk = pred_prob
    else:
        ml_risk = None

    # ---------- Build DB document ----------
    doc = {
        "node": obj.get("node"),
        "temp": float(obj.get("temp")) if obj.get("temp") is not None else None,
        "hum": float(obj.get("hum")) if obj.get("hum") is not None else None,
        "soil": float(obj.get("soil")) if obj.get("soil") is not None else None,
        "light": float(obj.get("light")) if obj.get("light") is not None else None,
        "gas": float(obj.get("gas")) if obj.get("gas") is not None else None,

        # optional rule-based value (legacy)
        "risk_rule": float(obj.get("risk")) if obj.get("risk") is not None else None,

        # ML outputs
        "ml_pred_label": pred_label,
        "ml_pred_prob": pred_prob,
        "ml_risk": ml_risk,

        # timestamps
        "timestamp": time.time(),
        "timestamp_ms": obj.get("timestamp_ms", int(time.time() * 1000))
    }

    res = mongo_coll.insert_one(doc)
    print(
    f"[OK] node={doc.get('node')} | "
    f"T={doc.get('temp')}°C | H={doc.get('hum')}% | "
    f"Soil={doc.get('soil')}% | Light={doc.get('light')} | Gas={doc.get('gas')} | "
    f"ML={doc.get('ml_pred_label')} | Prob={doc.get('ml_pred_prob')}"
    )

    return res


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--port", required=True)
    parser.add_argument("--baud", type=int, default=115200)
    parser.add_argument("--mongo", default=MONGO_URI)
    args = parser.parse_args()

    # MongoDB
    client = MongoClient(args.mongo)
    coll = client[DB_NAME][COLLECTION_NAME]
    print(f"[INFO] Connected to MongoDB: {DB_NAME}.{COLLECTION_NAME}")

    # Model
    model, label_enc = load_model()

    # Serial
    try:
        ser = serial.Serial(args.port, args.baud, timeout=1)
        print(f"[INFO] Listening on {args.port} @ {args.baud}")
    except Exception as e:
        print("[ERROR] Serial open failed:", e)
        sys.exit(1)

    try:
        while True:
            raw = ser.readline().decode("utf-8", errors="replace").strip()
            if not raw:
                continue

            parsed = parse_line(raw)
            if parsed is None:
                continue

            enrich_and_insert(coll, model, label_enc, parsed)

    except KeyboardInterrupt:
        print("\n[INFO] Stopped by user")
    finally:
        ser.close()


if __name__ == "__main__":
    main()
