
import re
import json
from groq import Groq
from openpipe import OpenAI

# Initialize Groq and OpenAI clients
client = Groq(api_key="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")
client_ai = OpenAI(
    openpipe={"api_key": "opk_06fc1dc079533e23f8eb1d69f8e8ebe2ef029aa9ce"}
)

question = "MODIFICACION DEL REGISTRO?"

# Load the data from the JSON file directly as a dictionary
with open("data.json", "r", encoding='utf-8') as f:
    json_data = json.load(f)

# headings = [
#     "SANIDAD EN VIVEROS FORESTALES",
#     "Que es una enfermedad",
#     "Que es una plaga",
#     "Que es Manejo",
#     "CLAVES PARA EL MANEJO",
#     "CLASES DE MANEJOS",
#     "RESISTENCIA",
#     "Causas de presencia de Enfermedades o plagas en un vivero",
#     "Enfermedades hongos",
#     "Prevencion de Enfermedades",
#     "Manejo de Enfermedades",
#     "FUNGICIDAS",
#     "BIOLOGICOS",
#     "DOSIS",
#     "Plagas",
#     "PREVENTIVOS CONTRA PLAGAS",
#     "CONTROL CONTRA PLAGAS",
#     "PLAGUICIDAS",
#     "APLICACION",
#     "CUIDADOS"
# ]

headings = list(json_data.keys())

# Print the list of keys
# print(keys)
# Normalize the headings by stripping whitespace
headings = [heading.strip() for heading in headings]

# Prompt for Groq API to identify relevant headings
prompt = f'''
You are given a list of keys of a json. For the given user question, find all relevant keys from that list that can hold the answer to the question and also donot add anything extra. just reply with what was asked.
Just return a list of those exact keys with proper and complete name as you got with double quotes and say nothing else. Return the keys like this ["key1", "key2"]
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
    
    # print(f"Extended Answer from Groq: {extended_answer}")
    
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


# Get the initial answer from OpenAI
completion = client_ai.chat.completions.create(
    model="openpipe:great-flies-win",
    messages=[
        {"role": "system", "content": "You are SILVIO, a tree production assistant"},
        {"role": "user", "content": question}
    ],
    temperature=0,
    openpipe={"tags": {"prompt_id": "counting", "any_key": "any_value"}},
)

# Get the answer from the API response (which contains YouTube links)
initial_answer = completion.choices[0].message.content
print("Initial Answer:", initial_answer)

# Load the description.json with YouTube links and their transcripts
with open('description.json', encoding='utf-8') as f:
    transcripts = json.load(f)  # Assuming {youtube_link: transcript}

# Function to extract URLs from the initial answer
def extract_urls(text):
    url_pattern = re.compile(r'https?://\S+')
    urls = url_pattern.findall(text)
    return urls

# Extract URLs from the initial answer
urls_in_answer = extract_urls(initial_answer)
print("URLs found in the answer:", urls_in_answer)

# Collect transcripts for each URL
import string

# Function to sanitize the URL by removing trailing spaces and punctuation
def sanitize_url(url):
    return url.rstrip(string.punctuation + ' ')

# Example data (your code remains unchanged except for the new sanitization logic)
tcs = []

# Sanitize the URLs in the answer and search them in transcripts
for url in urls_in_answer:
    clean_url = sanitize_url(url)  # Clean the URL by stripping unwanted characters
    if clean_url in transcripts:
        transcript = transcripts[clean_url]  # Use the sanitized URL as the key
        tcs.append([clean_url, transcript])



# Reframe the initial answer based on transcript relevance
def reframe_answer_based_on_transcripts(initial_answer, transcript_list, question,data):
    answer = initial_answer
    if len(data) > 800:
        data = data[:800]
    print('there')
    for i in transcript_list:
        transcript = i[1]
        prompt = (
                f'''
                User asked you a question, you are given data related to it from user provided pdfs. Answer user question from that data in complete spanish. Donot add anything from your own and be very specific to the data and question. Reply in friendly tone with some emojis. Give preccise and concise answer without talking about any data given to you.
                Question: {question}
                data: {data}
                '''
            )
        # prompt = (
        #         f'''
        #         User asked you a question, you are given initial answer generated by an llm. firstly validate the answer by the data given to you. Add all necessary correct info from data into answer. Donot miss any point involved in answering the question in data. Remember not to include any additional information that is not a part of the question. Just reply with what is asked.
        #          Secondly if transcript is not relevant to the answer, please remove the line that recommends url and reframe the answer, donot remove anything else. but if it is relevant return answer after just validating from data but recommend url staright forward and donot say extra when recommending. You need to provide the response such that user asked a question to you and now you are replying to it. Maintain firendly tone with some emojis.
        #         Do not talk about any revised answer or transcripts provided to you. Return a little shoter answer.
        #         Your answer should be very short and to the point with no additional information that is not asked by user.
        #         Question: {question}
        #         initial Answer: {initial_answer}
        #         Transcript: {transcript}
        #         data: {data}
        #         '''
        #     )

        
        try:
            # Generate completion from Groq API
            completion = client.chat.completions.create(
                model="llama3-8b-8192",
                messages=[
                    {"role": "system", "content": "You have two tasks. 1) validate the answer as per data. 2) check if the URL in the answer is suitable or not and then answer the user question as if they asked it from you."},
                    {"role": "user", "content": prompt}
                ],
                temperature=0.5,
                max_tokens=2048,
                top_p=1,
                stream=True,
                stop=None,
            )
            print('here')
            # Collect the response chunks
            extended_answer = ""
            for response_chunk in completion:
                extended_answer += response_chunk.choices[0].delta.content or ""
            print(f"Extended Answer from Groq: {extended_answer}")
            answer = extended_answer


        except Exception as e:
            print(f"Error extending answer: {e}")
            return answer  # Return the original answer in case of an error

    return answer

# Reframe the answer by removing URLs with irrelevant transcripts
reframed_answer = reframe_answer_based_on_transcripts(initial_answer, tcs, question,data)
print("Reframed Answer:", reframed_answer)

