from groq import Groq
import json

client = Groq(api_key="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")
question = "¿Qué es una enfermedad y qué es una plaga?"

# Load the data from the JSON file directly as a dictionary
with open("data.json", "r") as f:
    json_data = json.load(f)

headings = [
    "SANIDAD EN VIVEROS FORESTALES",
    "Que es una enfermedad",
    "Que es una plaga",
    "Que es Manejo",
    "CLAVES PARA EL MANEJO",
    "CLASES DE MANEJOS",
    "RESISTENCIA",
    "Causas de presencia de Enfermedades o plagas en un vivero",
    "Enfermedades (hongos)",
    "Prevención de Enfermedades",
    "Manejo de enfermedades",
    "FUNGICIDAS",
    "BIOLOGICOS",
    "DOSIS",
    "Plagas(insectos, roedores, moluscos, afidos, sinfilidos",
    "PREVENTIVOS CONTRA PLAGAS",
    "CONTROL CONTRA PLAGAS",
    "PLAGUICIDAS",
    "APLICACIÓN"
]

# Normalize the headings by stripping whitespace
headings = [heading.strip() for heading in headings]

# Prompt for Groq API to identify relevant headings
prompt = f'''
You are given a list of keys of a json. For the given user question, find relevant keys from that list that can hold the answer to the question.
Just return a list of those keys with double quotes and say nothing else. Return the keys like this ["key1", "key2"]
Question: {question}
List: {headings}
'''

try:
    # Generate completion from Groq API
    completion = client.chat.completions.create(
        model="llama3-8b-8192",
        messages=[{"role": "system", "content": "You are responsible to validate the answer from data"},
                  {"role": "user", "content": prompt}],
        temperature=0.5,
        max_tokens=2048,
        top_p=1,
        stream=True,
        stop=None,
    )
    
    # Collect the response chunks
    extended_answer = ""
    for response_chunk in completion:
        extended_answer += response_chunk.choices[0].delta.content or ""
    
    print(f"Extended Answer from Groq: {extended_answer}")
    
    # Process the answer to extract clean keys
    answer = extended_answer.strip("[]").split(",")
    answer = [item.strip().strip('"').strip() for item in answer]  # Remove quotes and whitespace

except Exception as e:
    print(f"Error extending answer: {e}")

# Retrieve and print data from json_data for each relevant key
data = ''
for key in answer:
    key = key.replace("'","")
    
    try:
        # print(f"{key}: {json_data[key]}")
        data+=json_data[key]
    except KeyError:
        print(f"Key '{key}' not found in json_data.")
