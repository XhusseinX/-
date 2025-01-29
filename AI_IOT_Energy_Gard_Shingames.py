import serial
import numpy as np
from sklearn.linear_model import SGDRegressor
from sklearn.pipeline import make_pipeline
from sklearn.preprocessing import StandardScaler
import time
import matplotlib.pyplot as plt
from sklearn.exceptions import NotFittedError

# Initialize models for each room and the house
def initialize_models():
    """Initialize SGD models for rooms and the house."""
    room_models = [make_pipeline(StandardScaler(), SGDRegressor(learning_rate='constant', eta0=0.0001)) for _ in range(4)]
    house_model = make_pipeline(StandardScaler(), SGDRegressor(learning_rate='constant', eta0=0.0001))
    return room_models, house_model

# Configure serial connection
def configure_serial(port='/dev/ttyUSB0', baud_rate=115200):
    """Configure and return a serial connection."""
    try:
        ser = serial.Serial(port, baud_rate, timeout=1)
        print(f"Serial connection established on {port}.")
        return ser
    except serial.SerialException as e:
        print(f"Error opening serial port: {e}")
        return None

# Update models incrementally
def update_models(sensor_values, room_models, house_model):
    """Update room and house models with new sensor data."""
    sensor_array = np.array(sensor_values).reshape(1, -1)  # Shape (1, 4)
    
    for i in range(4):
        X_room = np.array([[sensor_values[i]]])  # Single feature input
        target_room = sensor_values[i]  # Predict itself for now
        
        # Fit StandardScaler before first training iteration
        if not hasattr(room_models[i].named_steps['sgdregressor'], 'coef_'):
            room_models[i].named_steps['standardscaler'].fit(X_room)
        
        room_models[i].named_steps['sgdregressor'].partial_fit(X_room, [target_room])

    target_house = np.mean(sensor_values)  # Predict the average of all rooms
    
    if not hasattr(house_model.named_steps['sgdregressor'], 'coef_'):
        house_model.named_steps['standardscaler'].fit(sensor_array)
    
    house_model.named_steps['sgdregressor'].partial_fit(sensor_array, [target_house])

# Predict values for next day, week, and month
def predict_values(house_model, sensor_values):
    """Predict values for next day, week, and month."""
    X_house = np.array(sensor_values).reshape(1, -1)  # Shape (1, 4)
    try:
        house_prediction = house_model.predict(X_house)
        next_day_prediction = house_prediction[0] * 1.1 * 12 * 24 # Prediction for next day
        next_week_prediction = house_prediction[0] * 1.5 * 12 * 24 * 7  # Prediction for next week
        next_month_prediction = house_prediction[0] * 2.0 * 12 * 24 * 30 # Prediction for next month
        return next_day_prediction, next_week_prediction, next_month_prediction
    except NotFittedError:
        print("House model not yet trained.")
        return None, None, None

# Main loop for continuous prediction
def main():
    room_models, house_model = initialize_models()
    ser = configure_serial()
    
    if not ser:
        return  # Exit if serial connection fails
    
    print("Waiting for 'A' from ESP32 to start receiving sensor data...")
    while True:
        if ser.in_waiting > 0:
            data = ser.readline().decode('utf-8').strip()
            if data == 'A':
                print("Received 'A', starting data collection...")
                break

    ser.reset_input_buffer()
    time.sleep(1)
        
    # Initialize plot
    plt.ion()
    fig, ax = plt.subplots()
    lines = ax.plot([], [], [], [], [], [])
    ax.set_xlabel("Time")
    ax.set_ylabel("Predicted Values")
    ax.legend(["Next Day", "Next Week", "Next Month"])
    
    # Data storage for visualization
    prediction_history_size = 100
    next_day_predictions = []
    next_week_predictions = []
    next_month_predictions = []
        
    try:
        while True:
            if ser.in_waiting > 0:
                data = ser.readline().decode('utf-8').strip()
                
                if "Connecting to" in data or "Next Day" in data or "Predicted values" in data:
                    continue
                
                try:
                    sensor_values = list(map(float, data.split(',')))  # Change int to float
                    if len(sensor_values) != 4:
                        print(f"Invalid data format: {data}")
                        continue

                    update_models(sensor_values, room_models, house_model)
                    next_day, next_week, next_month = predict_values(house_model, sensor_values)

                    if next_day is not None:
                        next_day_predictions.append(next_day)
                        next_week_predictions.append(next_week)
                        next_month_predictions.append(next_month)
                        
                        if len(next_day_predictions) > prediction_history_size:
                            next_day_predictions.pop(0)
                            next_week_predictions.pop(0)
                            next_month_predictions.pop(0)
                        
                        print(f"Predicted values - Next Day: {next_day:.2f}, Next Week: {next_week:.2f}, Next Month: {next_month:.2f}")
                        
                        # Update plot
                        lines[0].set_data(range(len(next_day_predictions)), next_day_predictions)
                        lines[1].set_data(range(len(next_week_predictions)), next_week_predictions)
                        lines[2].set_data(range(len(next_month_predictions)), next_month_predictions)
                        ax.relim()
                        ax.autoscale_view()
                        plt.draw()
                        plt.pause(0.01)
                        
                        # Send predicted values back to ESP32
                        predicted_values = f"{next_day:.2f},{next_week:.2f},{next_month:.2f}"
                        ser.write((predicted_values + "\n").encode('utf-8'))
                        print("Predicted values sent to ESP32:", predicted_values)
                    
                except ValueError as e:
                    print(f"Error processing data: {e}. Data: {data}")
            
            time.sleep(0.1)
    
    except KeyboardInterrupt:
        print("Program terminated by user.")

if name == "__main__":
    main()
