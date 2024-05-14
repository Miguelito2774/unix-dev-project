def fibonacci(n):
    fib_series = [0, 1]
    while len(fib_series) < n:
        fib_series.append(fib_series[-1] + fib_series[-2])
    return fib_series


def main():
    n = 20
    fib_series = fibonacci(n)
    print("Serie de Fibonacci:")
    for num in fib_series:
        print(num, end=" ")


main()