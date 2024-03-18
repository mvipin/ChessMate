from gtts import gTTS
import subprocess
import os

# Function to convert WAV file using ffmpeg
def convert_wav(input_filename, output_filename):
    command = ['ffmpeg', '-i', input_filename, '-acodec', 'pcm_s16le', '-ar', '44100', output_filename]
    subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

# Load comments from a text file
with open('comments.txt', 'r') as file:
    comments = file.readlines()

# Process each comment
for i, comment in enumerate(comments):
    # Trim newline characters and any leading/trailing whitespace
    comment_cleaned = comment.strip()
    
    # Generate WAV file from the comment
    tts = gTTS(comment_cleaned, lang='en')
    temp_filename = f'comment_{i}_temp.wav'
    tts.save(temp_filename)
    
    # Convert the WAV file to a compatible format
    output_filename = f'comment_{i}.wav'
    convert_wav(temp_filename, output_filename)
    
    # Optional: Remove the temporary file
    os.remove(temp_filename)

    print(f"Processed comment {i+1}/{len(comments)}")

