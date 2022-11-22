import Blocks
import time

Blocks.setUser("antor.mee@gmail.com", "123456")

def main():
  prevTime = time.time
  buzzState = False
  
  while True:
    if Blocks.button() == 1:
      Blocks.LED(1)
    else:
      Blocks.LED(0)
    
    temp = Blocks.temp()
    if temp < 24: Blocks.relay(1)
    elif temp > 27: Blocks.relay(0)

    if time.time - prevTime > 5:
      buzzState = not buzzState
      Blocks.buzzer(buzzState)
      prevTime = time.time

if __name__ == '__main__':
  main()