import Blocks
import time

Blocks.setUser("antor.mee@gmail.com", "123456")

def main():
  while True:
    Blocks.LED(1)
    time.sleep(1)

    Blocks.buzzer(1)
    time.sleep(0.2)

    Blocks.LED(0)
    Blocks.buzzer(0)
    time.sleep(3)

if __name__ == '__main__':
  main()