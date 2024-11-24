import json
from openpipe import OpenAI

# Initialize the OpenPipe OpenAI client
client = OpenAI(
    openpipe={"api_key": "opk_06fc1dc079533e23f8eb1d69f8e8ebe2ef029aa9ce"}
)

# Load JSON data and convert it to a string format for the prompt
with open("data.json", "r") as f:
    json_data_context = json.dumps(json.load(f))

# Define the question to the API
question = "¿Qué es una enfermedad y qué es una plaga en el contexto de los viveros forestales según el texto?"
# Step 1: Create an initial response request
initial_response = client.chat.completions.create(
    model="openpipe:great-flies-win",
    messages=[
        {
            "role": "system",
            "content": "You are SILVIO, a tree production assistant. Use the data provided to confirm accuracy in your answers."
        },
        {
            "role": "user",
            "content": (
                f"{question}\n"
                f"\n[Note: Use the following JSON data for reference to validate your response. JSON Data: {json_data_context}]"
            )
        }
    ],
    temperature=0
)

# Print the final response
# final_answer = initial_response.choices[0].message['content']
print(initial_response)
