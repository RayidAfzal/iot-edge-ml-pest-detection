from flask import Flask, jsonify, render_template
from pymongo import MongoClient

app = Flask(__name__)

# Connect to MongoDB
client = MongoClient("mongodb://localhost:27017/")
db = client["farm_iot"]
collection = db["sensor_readings"]

@app.route("/")
def index():
    return render_template("index.html")

@app.route("/api/latest")
def api_latest():
    # Fetch last 100 readings (newest first)
    docs = list(collection.find().sort("timestamp", -1).limit(100))

    rows = []
    for d in docs:
        row = {
            "id": str(d.get("_id")),
            "timestamp": d.get("timestamp"),
            "timestamp_ms": d.get("timestamp_ms"),
            "node": d.get("node"),

            # sensor data
            "temp": d.get("temp"),
            "hum": d.get("hum"),
            "soil": d.get("soil"),
            "light": d.get("light"),
            "gas": d.get("gas"),

            # risks
            "ml_risk": d.get("ml_risk"),          # PRIMARY
            "risk_rule": d.get("risk_rule"),      # SECONDARY

            # ML details
            "ml_pred_label": d.get("ml_pred_label"),
            "ml_pred_prob": d.get("ml_pred_prob"),
        }
        rows.append(row)

    # Return oldest → newest for charts
    return jsonify(rows[::-1])

if __name__ == "__main__":
    print("Starting Flask server...")
    app.run(debug=True)
