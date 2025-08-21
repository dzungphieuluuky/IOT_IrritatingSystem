import numpy as np
import pandas as pd
from flask import Flask, request, jsonify
import requests
import pickle

class DecisionTree:
    def __init__(self, min_samples_split=2, max_depth=100, n_features=None):
        """
        Initializes the DecisionTree.

        Args:
            min_samples_split (int): The minimum number of samples required to split an internal node.
            max_depth (int): The maximum depth of the tree.
            n_features (int): The number of features to consider when looking for the best split.
        """
        self.min_samples_split = min_samples_split
        self.max_depth = max_depth
        self.n_features = n_features
        self.root = None

    def fit(self, X, y):
        """
        Builds the decision tree.

        Args:
            X (np.ndarray): The training input samples.
            y (np.ndarray): The target values.
        """
        self.n_features = X.shape[1] if not self.n_features else min(self.n_features, X.shape[1])
        self.root = self._grow_tree(X, y)

    def _grow_tree(self, X, y, depth=0):
        """
        Recursively grows the decision tree.

        Args:
            X (np.ndarray): The input samples for the current node.
            y (np.ndarray): The target values for the current node.
            depth (int): The current depth of the tree.

        Returns:
            dict: The grown (sub)tree.
        """
        n_samples, n_feats = X.shape

        # Stopping criteria
        if (depth >= self.max_depth or n_samples < self.min_samples_split or len(np.unique(y)) == 1):
            leaf_value = self._leaf_value(y)
            return {'leaf_value': leaf_value}

        feat_idxs = np.random.choice(n_feats, self.n_features, replace=False)
        best_feat, best_thresh = self._best_split(X, y, feat_idxs)

        left_idxs, right_idxs = self._split(X[:, best_feat], best_thresh)
        left = self._grow_tree(X[left_idxs, :], y[left_idxs], depth + 1)
        right = self._grow_tree(X[right_idxs, :], y[right_idxs], depth + 1)
        return {'feature_index': best_feat, 'threshold': best_thresh, 'left': left, 'right': right}

    def _best_split(self, X, y, feat_idxs):
        """
        Finds the best feature and threshold to split the data.

        Args:
            X (np.ndarray): The input samples.
            y (np.ndarray): The target values.
            feat_idxs (np.ndarray): The indices of the features to consider.

        Returns:
            tuple: The index of the best feature and the best threshold value.
        """
        best_gain = -1
        split_idx, split_thresh = None, None
        for feat_idx in feat_idxs:
            X_column = X[:, feat_idx]
            thresholds = np.unique(X_column)
            for thr in thresholds:
                gain = self._variance_reduction(y, X_column, thr)
                if gain > best_gain:
                    best_gain = gain
                    split_idx = feat_idx
                    split_thresh = thr
        return split_idx, split_thresh

    def _variance_reduction(self, y, X_column, split_thresh):
        """
        Calculates the variance reduction for a given split.

        Args:
            y (np.ndarray): The target values.
            X_column (np.ndarray): The feature column to split on.
            split_thresh (float): The threshold for the split.

        Returns:
            float: The variance reduction.
        """
        parent_variance = np.var(y)
        left_idxs, right_idxs = self._split(X_column, split_thresh)
        if len(left_idxs) == 0 or len(right_idxs) == 0:
            return 0
        n = len(y)
        n_l, n_r = len(left_idxs), len(right_idxs)
        var_l, var_r = np.var(y[left_idxs]), np.var(y[right_idxs])
        child_variance = (n_l / n) * var_l + (n_r / n) * var_r
        vr = parent_variance - child_variance
        return vr

    def _split(self, X_column, split_thresh):
        """
        Splits a feature column based on a threshold.

        Args:
            X_column (np.ndarray): The feature column to split.
            split_thresh (float): The threshold value.

        Returns:
            tuple: Indices of the left and right splits.
        """
        left_idxs = np.argwhere(X_column <= split_thresh).flatten()
        right_idxs = np.argwhere(X_column > split_thresh).flatten()
        return left_idxs, right_idxs

    def _leaf_value(self, y):
        """
        Calculates the value of a leaf node (mean of target values).

        Args:
            y (np.ndarray): The target values in the leaf node.

        Returns:
            float: The mean of the target values.
        """
        return np.mean(y)

    def predict(self, X):
        """
        Makes predictions for a set of input samples.

        Args:
            X (np.ndarray): The input samples.

        Returns:
            np.ndarray: The predicted values.
        """
        return np.array([self._traverse_tree(x, self.root) for x in X])

    def _traverse_tree(self, x, node):
        """
        Recursively traverses the tree to make a prediction for a single sample.

        Args:
            x (np.ndarray): A single input sample.
            node (dict): The current node in the tree.

        Returns:
            float: The predicted value.
        """
        if 'leaf_value' in node:
            return node['leaf_value']

        if x[node['feature_index']] <= node['threshold']:
            return self._traverse_tree(x, node['left'])
        return self._traverse_tree(x, node['right'])

class RandomForest:
    def __init__(self, n_trees=100, min_samples_split=2, max_depth=100, n_features=None):
        """
        Initializes the RandomForest.

        Args:
            n_trees (int): The number of trees in the forest.
            min_samples_split (int): The minimum number of samples required to split an internal node.
            max_depth (int): The maximum depth of the trees.
            n_features (int): The number of features to consider when looking for the best split.
        """
        self.n_trees = n_trees
        self.min_samples_split = min_samples_split
        self.max_depth = max_depth
        self.n_features = n_features
        self.trees = []

    def fit(self, X, y):
        """
        Trains the random forest by fitting individual decision trees.

        Args:
            X (np.ndarray): The training input samples.
            y (np.ndarray): The target values.
        """
        self.trees = []
        for _ in range(self.n_trees):
            tree = DecisionTree(
                min_samples_split=self.min_samples_split,
                max_depth=self.max_depth,
                n_features=self.n_features)
            X_sample, y_sample = self._bootstrap_samples(X, y)
            tree.fit(X_sample, y_sample)
            self.trees.append(tree)

    def _bootstrap_samples(self, X, y):
        """
        Creates a bootstrapped sample of the data.

        Args:
            X (np.ndarray): The input samples.
            y (np.ndarray): The target values.

        Returns:
            tuple: A bootstrapped sample of X and y.
        """
        n_samples = X.shape[0]
        idxs = np.random.choice(n_samples, n_samples, replace=True)
        return X[idxs], y[idxs]

    def predict(self, X):
        """
        Makes predictions by averaging the predictions of all trees in the forest.

        Args:
            X (np.ndarray): The input samples.

        Returns:
            np.ndarray: The predicted values.
        """
        tree_preds = np.array([tree.predict(X) for tree in self.trees])
        return np.mean(tree_preds, axis=0)

def generate_dummy_data(num_samples=200):
    # Simulate humidity in % (0 to 100)
    data = []

    for _ in range(num_samples):
        humidity = np.random.uniform(0, 100)  # percentage
        temp = np.random.uniform(15, 40)      # °C
        light = np.random.uniform(0, 4096)    # lux

        # Rule: If humidity is high enough, no watering
        if humidity > 70:
            water_time = 0
        else:
            # Base water time depends on dryness
            dryness_factor = (70 - humidity) / 70  
            # Temperature increases need for water
            temp_factor = (temp - 15) / 25  
            # Light increases evaporation
            light_factor = light / 4096  

            # Scale factors into a water time (seconds)
            water_time = int((dryness_factor + temp_factor*0.5 + light_factor*0.3) * 30)
            water_time = max(water_time, 0)

        data.append([humidity, temp, light, water_time])

    df = pd.DataFrame(data, columns=["humid", "temp", "light", "water_time"])
    return df

# Example usage
df = generate_dummy_data(200)
print(df.head())

THINGSPEAK_CHANNEL_ID = "YOUR_CHANNEL_ID"
THINGSPEAK_READ_KEY = "YOUR_READ_API_KEY"
THINGSPEAK_RESULTS = 500  # number of recent records to fetch
app = Flask(__name__)

def load_data_from_thingspeak():
    url = f"https://api.thingspeak.com/channels/{THINGSPEAK_CHANNEL_ID}/feeds.json?api_key={THINGSPEAK_READ_KEY}&results={THINGSPEAK_RESULTS}"
    response = requests.get(url)
    data = response.json()

    # Convert feeds to DataFrame
    feeds = data["feeds"]
    df = pd.DataFrame(feeds)

    # Rename ThingSpeak's field columns to match your model’s expectation
    df = df.rename(columns={
        "field1": "humid",
        "field2": "temp",
        "field3": "light",
        "field4": "water_level",
        "field5": "water_time"
    })

    # Convert to numeric & drop missing values
    for col in ["humid", "temp", "light", "water_level", "water_time"]:
        df[col] = pd.to_numeric(df[col], errors="coerce")

    df = df.dropna()

    return df


def train_model(df):
    X = df[["humid", "temp", "light"]].values
    y = df["water_time"].values

    water_time_model = RandomForest(n_features=3)
    water_time_model.fit(X, y)
    with open("water_time_model.pkl", "wb") as f:
        pickle.dump(water_time_model, f)
    return water_time_model

@app.route("/predict", methods=["POST"])
def predict():
    try:
        data = request.get_json()
        for key in ["humid", "temp", "light"]:
            if key not in data:
                return jsonify({"error": f"Missing {key}"}), 400

        input_data = np.array([[data["humid"], data["temp"], data["light"]]])

        water_time = water_time_model.predict(input_data)[0]
        water_time = int(water_time)
        return jsonify({
            "water_time": water_time
        })
    except Exception as e:
        return jsonify({"error": str(e)}), 400

if __name__ == "__main__":
    train = True if input("Training phase? (y/n): ").lower() == "y" else False
    if train:
        print("Loading data and training model...")
        # data_df = load_data_from_thingspeak()
        data_df = generate_dummy_data()
        water_time_model = train_model(data_df)
    else:
        with open("water_time_model.pkl", "rb") as f:
            water_time_model = pickle.load(f)
        app.run(host="0.0.0.0", port=5000)
