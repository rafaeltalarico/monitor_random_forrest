import pandas as pd
from sklearn.ensemble import RandomForestRegressor
from sklearn.model_selection import train_test_split
from sklearn.metrics import mean_squared_error
from Prediction.data_analyzes import DataAnalyzes

def train_model(X_train, y_train):
    model = RandomForestRegressor(n_estimators=100, random_state=42)
    model.fit(X_train, y_train)
    return model

def evaluate_model(model, X_test, y_test):
    predictions = model.predict(X_test)
    mse = mean_squared_error(y_test, predictions)
    print(f"Mean Squared Error: {mse}")

def main():
    dataAnalyzes = DataAnalyzes()
    dataAnalyzes.analyze()

    data = dataAnalyzes.dataChapadaAraripe
    
    X = data.drop('target_column', axis=1)
    y = data['target_column']
    
    X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.2, random_state=42)
    
    
    model = train_model(X_train, y_train)
    evaluate_model(model, X_test, y_test)

if __name__ == '__main__':
    print("Modelo sendo iniciado")
    main()
    print("Modelo finalizado")
