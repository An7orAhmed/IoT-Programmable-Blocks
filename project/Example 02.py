import Blocks

Blocks.setUser("antor.mee@gmail.com", "123456")

def main():
  while True:
    if Blocks.button() == 1:
      Blocks.buzzer(1)
      Blocks.relay(1)
    else:
      Blocks.buzzer(0)
      Blocks.relay(0)

if __name__ == '__main__':
  main()