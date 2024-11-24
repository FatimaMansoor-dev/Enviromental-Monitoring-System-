import json
with open("data.json", "r") as f:
    json_data = json.load(f)
print(json_data["Que es una enfermedad"])