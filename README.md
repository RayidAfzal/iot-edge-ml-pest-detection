# 🌱 Edge-Intelligent Pest Monitoring System

An edge-intelligent IoT system for real-time agricultural pest monitoring.  
Environmental data from multiple sensor nodes is processed locally at an edge gateway, where a machine learning model predicts pest conditions and associated risk with low latency.

---

## 🚜 Problem Statement

Traditional pest monitoring systems rely on manual inspection or cloud-based analytics, which often suffer from high latency, continuous internet dependency, and delayed response times.

This project demonstrates how **edge intelligence** can be used to perform real-time pest prediction close to the data source, making the system faster, scalable, and more reliable.

---

## 🏗️ System Architecture

**High-level pipeline:**

1. **IoT Sensor Nodes**
   - ESP-based nodes deployed per crop (Tomato, Carrot)
   - Sensors:
     - Temperature & Humidity (DHT11)
     - Soil Moisture (Capacitive)
     - Light Intensity (LDR)
     - Gas Concentration (MQ-135)

2. **Edge Gateway**
   - Receives sensor data via serial communication
   - Runs a local Python backend
   - Handles data parsing, feature extraction, and ML inference

3. **Machine Learning Inference**
   - Trained Random Forest model
   - Predicts pest type and associated risk probability
   - Runs locally (edge-level processing)

4. **Database**
   - MongoDB stores raw sensor data and ML predictions

5. **Web Dashboard**
   - Flask-based backend
   - Live visualization of sensor trends and ML-based pest risk

---

## 🧠 Machine Learning Details

- **Algorithm:** Random Forest Classifier  
- **Framework:** scikit-learn  

**Why Random Forest?**
- Robust to noisy sensor data  
- Performs well on small-to-medium datasets  
- Fast inference suitable for edge deployment  
- Interpretable compared to deep learning models  

**Model Inputs:**
- Gas concentration (MQ-135)
- Temperature
- Humidity
- Soil moisture
- Crop type (encoded)

**Model Outputs:**
- Predicted pest type (e.g., No Pest, Leaf Miner Attack)
- Pest risk probability

---

## 💾 Data Pipeline

1. Sensor nodes transmit JSON-formatted data to the gateway
2. Gateway backend:
   - Parses incoming data
   - Extracts ML features
   - Runs model inference
3. Enriched data is stored in MongoDB
4. Dashboard fetches and visualizes data in real time

---

## 🖥️ Dashboard Features

- Real-time sensor value visualization
- ML-based pest risk monitoring
- Crop-wise comparison (Tomato vs Carrot)
- Historical data tracking

---

## 🧰 Technologies Used

- **Hardware:** ESP8266 / ESP32, DHT11, MQ-135, Soil Moisture Sensor, LDR  
- **Backend:** Python, Flask  
- **Machine Learning:** scikit-learn  
- **Database:** MongoDB  
- **Frontend:** HTML, CSS, Chart.js  
- **Communication:** Serial (UART), JSON  

---

## 🎯 Key Outcomes

- Demonstrated practical edge AI for agriculture
- Reduced reliance on cloud services
- Achieved low-latency pest prediction
- Built a complete end-to-end IoT + ML system

---

## 📌 Future Improvements

- On-device ML inference using TensorFlow Lite
- Support for additional crops and pest classes
- Improved sensor calibration and model accuracy
- Alerting system for high-risk pest events

---
