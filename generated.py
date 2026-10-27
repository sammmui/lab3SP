def main():
    x = int(1)
    result = int(1)
    while (x <= 5):
        result = int((result * x))
        x = int((x + 1))
    if (result == 120):
        print(result)
    else:
        print(0)

if __name__ == "__main__":
    main()
