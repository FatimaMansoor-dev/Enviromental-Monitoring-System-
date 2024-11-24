from groq import Groq
import json

# Initialize Groq and OpenAI clients
client = Groq(api_key="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")


question = '¿Cual es la profundidad de seimbra que debo colocar las semillas?'

# Load the data from the JSON file directly as a dictionary
with open("data.json", "r", encoding='utf-8') as f:
    json_data = json.load(f)


headings = list(json_data.keys())
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
        messages=[{"role": "system", "content": "You are responsible to find relevant headings that can help answer question"},
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


data = ''
for key in answer:
    key = key.replace("'","")
    
    try:
        # print(f"{key}: {json_data[key]}")
        data+=json_data[key]
    except KeyError:
        print(f"Key '{key}' not found in json_data.")

# Load the description.json with YouTube links and their transcripts
with open('vids.json', encoding='utf-8') as f:
    urls = json.load(f) 


if len(data) > 800:
    data = data[:800]
    prompt = (
            f'''
            User asked you a question, you are given data related to it from user provided pdfs. Answer user question from that data in complete spanish. Donot add anything from your own and be very specific to the data and question. Reply in friendly tone with some emojis. Give preccise and concise answer without talking about any data given to you.
            Secondly, you may reccommend any url along with the link that you think will be useful to user for their question from the urls given to you. But the link should be really specific to the topic. If you find no link, just return asnwer without it. Donot say i didnt find any link.
            Question: {question}
            data: {data}
            urls: {urls}
            '''
        )
    try:
        # Generate completion from Groq API
        completion = client.chat.completions.create(
            model="llama3-8b-8192",
            messages=[
                {"role": "system", "content": "You have two tasks. 1) Give relevant answer as per data. 2) Check if any url relates"},
                {"role": "user", "content": prompt}
            ],
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
        answer = extended_answer


    except Exception as e:
        print(f"Error extending answer: {e}")

print(answer)



        