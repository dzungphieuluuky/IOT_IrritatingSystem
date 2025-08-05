# 🌱 Smart Auto Irrigation System

This project is an **IoT-based smart irrigation system** that automates plant watering using real-time sensor data and machine learning. The system uses an **ESP32**, **soil moisture sensors**, and **relay-controlled water pump**, connected to a **Node-RED dashboard** with a **Python-based ML model** to make intelligent decisions.

---

## 🔧 Features

* 📡 **Real-time Sensor Monitoring** (Soil Moisture, Temperature, etc.)
* 🤖 **Automatic Pump Control** via trained ML model
* 📈 **Data Storage in Cloud**
* 🧠 **Model Training using historical sensor data**
* 🧪 **Manual Watering Option** from Node-RED Dashboard
* 🔔 **Recent Notifications Panel** (Last 5 actions)
* 💻 **Custom Node-RED UI** for control and monitoring

---

## 🛠️ Technologies Used

| Tech               | Purpose                            |
| ------------------ | ---------------------------------- |
| **ESP32**          | Sensor reading & relay control     |
| **Node-RED**       | Flow-based dashboard & API routing |
| **Python (Flask)** | Machine Learning API server        |
| **Relay Module**   | Controls the pump via GPIO         |

---

## 📦 System Architecture

1. ESP32 collects sensor data and sends it to Node-RED via MQTT.
2. Node-RED saves data to Cloud and calls the ML API for predictions.
3. ML API predicts watering time based on recent data.
4. Node-RED sends a signal to ESP32 to activate the pump accordingly.
5. User can override using **manual watering toggle** on the dashboard.
6. Notifications (e.g., "Pump Activated at 23:57") are logged and displayed.

---

## 🖥️ Dashboard Highlights

* Real-time moisture levels
* Toggle button for manual watering
* Pump status indicator
* Last 5 notifications with timestamps
* Watering history logs

---

## 🚀 Setup Instructions

### 1. ESP32

* Connect sensors & relay
* Flash ESP32 with `esp32-code.ino`

### 2. Node-RED

* Import `node-red-flows.json`
* Configure HTTP and InfluxDB nodes

### 3. ML API

```bash
cd ml_api
pip install -r requirements.txt
python ml_api.py
```

## 🧠 Model Training (optional)

You can retrain the model using stored sensor data in InfluxDB:

```python
# In ml_api/train_model.py
python train_model.py
```

---

## 🤝 Future Improvements

* Add notification via email
* Integrate rain prediction
* Battery and solar power support
* Long-term analytics dashboard

---

## 📝 License

MIT License

---
