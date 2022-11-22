import requests
import smtplib, ssl
from email import encoders
from email.mime.base import MIMEBase
from email.mime.multipart import MIMEMultipart
from email.mime.text import MIMEText

sender_email = "***********"
email_password = "***********"
#-------------------------
host = "https://esinebd.com/projects/IoTBlocks/index.php"
did = ['button', 'led', 'buzzer', 'temp', 'relay']
email, password = "", ""

def setUser(em, pa):
  global email, password
  email = em
  password = pa
  print(f"Logged in as: {email}")

def button():
  global email, password
  reqParam = {'email': email, 'pass' : password}
  reqParam['get'] = ''
  reqParam['deviceId'] = did[0]
  resp = requests.get(host, params=reqParam)
  json = resp.json()
  resp.close()
  return int(json[0]['state'])

def temp(*arg):
  global email, password
  if(len(arg) == 0):
    reqParam = {'email': email, 'pass' : password}
    reqParam['get'] = ''
    reqParam['deviceId'] = did[3]
    resp = requests.get(host, params=reqParam)
    json = resp.json()
    resp.close()
    return float(json[0]['state'])

def LED(*arg):
  global email, password
  if(len(arg) == 0):
    reqParam = {'email': email, 'pass' : password}
    reqParam['get'] = ''
    reqParam['deviceId'] = did[1]
    resp = requests.get(host, params=reqParam)
    json = resp.json()
    resp.close()
    return int(json[0]['state'])
  else:
    reqParam = {'email': email, 'pass' : password}
    reqParam['set'] = ''
    reqParam['deviceId'] = did[1]
    reqParam['state'] = str(arg[0])
    resp = requests.get(host, params=reqParam)
    json = resp.json()
    resp.close()
    return 0

def buzzer(*arg):
  global email, password
  if(len(arg) == 0):
    reqParam = {'email': email, 'pass' : password}
    reqParam['get'] = ''
    reqParam['deviceId'] = did[2]
    resp = requests.get(host, params=reqParam)
    json = resp.json()
    resp.close()
    return int(json[0]['state'])
  else:
    reqParam = {'email': email, 'pass' : password}
    reqParam['set'] = ''
    reqParam['deviceId'] = did[2]
    reqParam['state'] = str(arg[0])
    resp = requests.get(host, params=reqParam)
    json = resp.json()
    resp.close()
    return 0

def relay(*arg):
  global email, password
  if(len(arg) == 0):
    reqParam = {'email': email, 'pass' : password}
    reqParam['get'] = ''
    reqParam['deviceId'] = did[4]
    resp = requests.get(host, params=reqParam)
    json = resp.json()
    resp.close()
    return int(json[0]['state'])
  else:
    reqParam = {'email': email, 'pass' : password}
    reqParam['set'] = ''
    reqParam['deviceId'] = did[4]
    reqParam['state'] = str(arg[0])
    resp = requests.get(host, params=reqParam)
    json = resp.json()
    resp.close()
    return 0

def sendEmail(email, subject, body):
  message = MIMEMultipart()
  message["From"] = sender_email
  message["To"] = email
  message["Subject"] = subject

  message.attach(MIMEText(body, "plain"))
  print(">> Sending Mail...")
  try:
    text = message.as_string()
    context = ssl.create_default_context()
    with smtplib.SMTP_SSL("mail.esinebd.com", 465, context=context) as server:
      server.login(sender_email, email_password)
      server.sendmail(sender_email, email, text)
      print(">> Mail Sent.")
  except Exception as e:
    print(e)