import pickle
loaded_model = pickle.load(open("filename.pickle", "rb"))
T = [[50, 50, 1300]] # input humidity temperature smoke
y_predicted = loaded_model.predict(T)
print(y_predicted)