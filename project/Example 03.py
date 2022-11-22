import Blocks

Blocks.setUser("antor.mee@gmail.com", "123456")

receiver = "alsabid02@gmail.com"
subject = "IoT Blocks Reporting"

def main():
  isEmailSent = False
  
  while True:
    temp = Blocks.temp()
    if temp < 29 and isEmailSent == False: 
      Blocks.sendEmail(receiver, subject, f"Temperature is Cold! {temp}")
      isEmailSent = True
    elif temp > 32 and isEmailSent == False: 
      Blocks.sendEmail(receiver, subject, f"Temperature is Hot! {temp}")
      isEmailSent = True
    elif temp >= 29 and temp <= 32: 
      isEmailSent = False

if __name__ == '__main__':
  main()