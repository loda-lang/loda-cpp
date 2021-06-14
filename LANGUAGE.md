# The LODA Language

The LODA language is an assembly language with instructions for common integer operations. It supports an unbounded set of memory cells storing integer, arithmetic operations and a loop based on a lexicographical order descent on memory regions.

Here is a basic example of a LODA program:

```asm
; A000045: Fibonacci numbers: F(n) = F(n-1) + F(n-2) with F(0) = 0 and F(1) = 1.

; argument is stored in $0

mov $3,1      ; assign $3:=1
lpb $0        ; loop as long as $0 decreases and is non-negative
  sub $0,1    ; decrement $0
  mov $2,$1   ; assign $2:=$1
  add $1,$3   ; add $1:=$1+$3
  mov $3,$2   ; assign $3:=$2
lpe           ; end of loop

; result is stored in $1
```

To run this program, you can run:
```bash
./loda eval A45
0,1,1,2,3,5,8,13,21,34,55,89,144,233,377,610,987,1597,2584,4181
```

The details of the language are explained in the following sections.

### Memory

Programs operate on memory consisting of an unbounded sequence of memory cells `$0`,`$1`,`$2`,... each storing an integer. There are three types of operands supported:

1. __Constants__, for example 5.
2. __Direct memory access__, for example `$5`. Reads or writes the value of the fifth memory cell.
3. __Indirect memory access__, for example `$$7`. Reads the value at memory cell #7 and interprets it as an address. For instance, if the value of `$7` is 13, then `$$7` accesses the memory cell #13.

### Arithmetic Operations

The table below summarizes the operations currently supported by LODA. We use the [Intel assembly syntax](https://en.wikipedia.org/wiki/X86_assembly_language), i.e., target before source. In the following, let `a` be a direct or an indirect memory access, and let `b` be a constant, a direct or an indirect memory access.

| Operation | Name           | Description |
|:---------:|:--------------:|-------------|
| `mov a,b` | Assignment     | Assign the value of the source to the target operand: `a:=b` |
| `add a,b` | Addition       | Add the source to the target operand: `a:=a+b` |
| `sub a,b` | Subtraction    | Subtract the source from the target operand: `a:=a-b` |
| `trn a,b` | Truncation     | Subtract and ensure non-negative result: `a:=max(a-b,0)` |
| `mul a,b` | Multiplication | Multiply the target with the source value: `a:=a*b` |
| `div a,b` | Division       | Divide the target by the source value: `a:=floor(a/b)`  |
| `dif a,b` | Divide-If-Divides | Divide the target by the source value if the source is a divisor: `a:=(a%b=0)?a/b:a `  |
| `mod a,b` | Modulus        | Remainder of division of target by source: `a:=a%b` |
| `pow a,b` | Power          | Raise source to the power of target: `a:=a^b` |
| `gcd a,b` | Greatest Common Divisor | Greatest common divisor or target and source: `a:=gcd(a,b)`. Undefined for 0,0. Otherwise always positive. |
| `bin a,b` | Binomial Coefficient | Target over source: `a:=a!/(b!(a-b)!)`|
| `cmp a,b` | Comparison     | Compare target with source: `a:=(a=b)?1:0` |
| `min a,b` | Minimum        | Minimum of target and source: `a:=min(a,b)` |
| `max a,b` | Maximum        | Maximum of target and source: `a:=max(a,b)` |

There is an additional (non-arithmetic) operation called `clr` ("clear"). It sets a memory region to zero. The target operand marks the start of the memory region. The second argument is interpreter as length of the memory region. For example `clr $2,3` sets the memory cells `$2`,`$3``$4` to zero.

### Loops and Conditionals

Loops are implemented as code blocks between `lpb` and `lpe` operations. The block is executed as long as a variable is decreasing and non-negative. For example, consider the following program:

```asm
mov $1,1
lpb $0
  mul $1,5
  sub $0,1
lpe
```

It first assigns 1 to the output cell `$1`. Inside the loop, the input cell `$0` is counted down to zero and in every step `$1` is multiplied by 5. Note that this could be also achieved without loops using the `pow` operation. Note that if the loop counter is not decreasing or becomes negative, the side effects of this last iteration are undone. This also enables the usage of this concept as conditional. For example, the following code multiplies `$1` by 5 if `$0` is greater than `17`.

```asm
lpb $0
  mul $1,5
  mov $0,17
lpe
```

The `lpb` can also have a second (optional) argument. In that case, the loop counter is not a single variable, but a finite memory region, which must strictly decreases in every iteration of the loop. The loop counter cell marks the start of that memory region, whereas the second argument is interpreted as a number and defines the length of this region. For example, `lpb $4,3` ... `lpe` is executed as long as the vector (or polynomial) `$4`,`$5`,`$6` is non-negative and strictly decreasing in every iteration according to the lexicographical ordering. If `y` is not a constant and evaluates to different values in subsequent iterations, the minimum length is used to compare the memory regions.

### Calls

Calling another LODA program is supported using the `cal` operation. This assumes you are evaluating the program as a sequence (see below). This operation takes two arguments. The first one is the parameter of the called program. The second argument is the number of the OEIS program to be called (see below). The result is stored in the first argument. For example, the operation `cal $2,45` evaluates the program A000045 (Fibonacci numbers) using the argument value in `$2` and overrides it with the result.

### Termination

All LODA programs are guaranteed to halt on every input. Recursive calls are not allowed. An infinite loop also cannot occur, because the values of the memory region strictly decrease in every iteration and can at most reach the region consisting only of zeros. Hence, all loops therefore also all LODA programs eventually terminate.

### Integer Sequences

Programs operate on an unbounded set of memory cells. To compute integer sequences, we consider `$0` as the input and `$1` as the output of the program. Thus, a sequence `a(n)` is generated by taking `$0=n` as input and producing the output `a(n)=$1`.
