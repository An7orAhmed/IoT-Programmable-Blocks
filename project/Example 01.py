import Blocks

Blocks.setUser("antor.mee@gmail.com", "123456")

def main():
  while True:
    if Blocks.button() == 1:
      Blocks.LED(1)
    else:
      Blocks.LED(0)

if __name__ == '__main__':
  main()