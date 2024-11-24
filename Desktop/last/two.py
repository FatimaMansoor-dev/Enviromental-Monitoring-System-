import re
import json
from groq import Groq
from openpipe import OpenAI

# Initialize Groq and OpenAI clients
client = Groq(api_key="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")
client_ai = OpenAI(
    openpipe={"api_key": "opk_06fc1dc079533e23f8eb1d69f8e8ebe2ef029aa9ce"}
)

question = "¿Qué características tiene la madera de Prosopis juliflora?"

# Load JSON data and convert it to a string format for the prompt
with open("data.json", "r") as f:
    json_data_context = json.dumps(json.load(f))

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
    print('there')
    for i in transcript_list:
        transcript = i[1]
        prompt = (
                f'''
                For the provided user question, firstly validate the answer by the data given to you.
                 Secondly if transcript is not relevant, please remove the line that recommends url and reframe the answer, donot remove anything else. else return answer as it is but recommend url staright forward and donot say extra when recommending. You need to provide the response such that user asked a question to you and now you are replying to it. 
                Do not talk about any revised answer or transcripts provided to you. Return a little shoter answer.
                Question: {question}
                Answer: {initial_answer}
                Transcript: {transcript}
                data: {data}
                '''
            )

        
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
reframed_answer = reframe_answer_based_on_transcripts(initial_answer, tcs, question)
print("Reframed Answer:", reframed_answer)

