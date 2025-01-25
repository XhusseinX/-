import serial
import numpy as np
from sklearn.linear_model import SGDRegressor
from collections import deque
import joblib
import os

# Initialize the online learning model
model = SGDRegressor(learning_rate='constant', eta0=0.01)  # Adjust learning rate as needed

# Sliding window to store the last 60 sensor values
window_size = 60
sensor_window = deque(maxlen=window_size)

# Configure serial connection
ser = serial.Serial('/dev/ttyS0', 115200, timeout=1)  # Use '/dev/ttyAMA0' or '/dev/serial0' if needed

print("Waiting for sensor data...")

while True:
    if ser.in_waiting > 0:
        # Read sensor data from ESP32
        data = ser.readline().decode('utf-8').strip()
        try:
            sensor_value = int(data)  # Convert data to integer
            print(f"Sensor Reading: {sensor_value}")

            # Add the new sensor value to the window
            sensor_window.append(sensor_value)

            # Check if the window has 60 values
            if len(sensor_window) == window_size:
                # Convert the window to a numpy array for prediction
                X = np.array(sensor_window).reshape(1, -1)  # Reshape for model input

                # Make prediction
                prediction = model.predict(X)
                print(f"Prediction: {prediction[0]}")

                # Update the model with the new data point
                # For now, assume the target value is the average of the window (replace with your logic)
                target_value = np.mean(sensor_window)
                model.partial_fit(X, [target_value])

                # Save the model to the desktop
                desktop_path = os.path.join(os.path.expanduser('~'), 'Desktop')
                model_filename = os.path.join(desktop_path, 'sliding_window_model.pkl')
                joblib.dump(model, model_filename)
                print(f"Model saved to: {model_filename}")

                # Send prediction back to ESP32 (optional)
                ser.write(f"{prediction[0]}\n".encode('utf-8'))
        except ValueError:
            print("Invalid data received")