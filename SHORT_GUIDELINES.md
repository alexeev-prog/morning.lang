# Complete Guide to the Morning Programming Language

Morning is a modern language based on S-expressions and reminiscent of Lisp. It offers flexibility, a simple syntax, and powerful capabilities for writing code. This guide presents the main constructs of the language, their usage, and best practices.

## 1. Code Structure

Code in Morning is organized using square brackets for main constructs, while round parentheses are used for parameters and expressions. This simplifies the perception of the code and promotes maintainability.

**Basic syntax example:**

```morning
[func exampleFunction (param)
    [scope
        // Your code here
    ]
]
```

## 2. Keywords and Capabilities

### 2.1. Function Definition with func

Functions are declared using the keyword func. The function name should be written in lower_case. You can specify parameters as well as a return type using the operator ->.

- **Example of a non-typed function:**

```
[func print_message ((msg !str))
    [fprint "%s\n" msg]
]
```

- **Example of a typed function:**

```
[func add_numbers ((a !int) (b !int)) -> !int
    [return (+ a b)]
]
```

> Note: It's good practice to use descriptive names for functions and parameters to make the code more understandable for other developers.

### 2.2. Variable Declaration with var and Modification with set

var is used for declaring variables with a specified type, while set is used for modifying their values. This helps maintain code clarity and simplifies debugging.

**Example of declaring a variable:**

```
[var (age !int) 25] // Declare the variable age of type !int and initialize it with the value 25
```

**Modifying the value of a variable:**

```
[set age (+ age 1)] // Increment the value of age by 1
```

> Note: Always initialize variables when declaring them to avoid runtime errors.

### 2.3. Conditions with check

The check construct allows the implementation of conditional operations, supporting both if-then and if-then-else constructs.

**Example of if-then:**

```
[check (> age 18)
    [fprint "Adult\n"] // Block executes if age is greater than 18
]
```

**Example of if-then-else:**

```
[check (> age 18)
    [fprint "Adult\n"]     // then
    [fprint "Minor\n"]     // else
]
```

> Note: Use if-then constructs when you do not need to handle an alternative case. This simplifies the code and makes it easier to read.

### 2.4. Loops with while

The while loop executes a block of code as long as the condition is true. This allows you to perform repeating actions without duplicating code.

**Example:**

```
[var (count !int) 0]
[while (< count 5)
    [scope
        [fprint "Count: %d\n" count] // Output the current value of count
        [set count (+ count 1)]       // Increment count by 1 after each iteration
    ]
]
```

> Note: Ensure that the loop has exit conditions to avoid infinite loops that may cause the program to hang.

### 2.5. Output with fprint

The fprint function allows for formatted output of data to the console, similar to printf in C.

**Example of usage:**

```
[fprint "Hello, %s! You are %d years old.\n" name age]
```

> Note: Use fprint to present data in a well-understood graphic or text format to improve the user experience.

### 2.6. Classes and Methods

Morning supports object-oriented programming, allowing you to create classes for organizing and structuring code. It is important to note that the OOP functionality in the language is still under development.

**Defining a class using class:**

```
[class Person self
    [scope
        (var name !string) // Property name
        (var age !int)     // Property age

        // Constructor initializing the object's properties
        (func init (this n a)
            [scope
                [set (property this name) n] // Initialize property name
                [set (property this age) a]   // Initialize property age
            ]
        )

        // Method for printing a greeting message
        (func greet (this)
            [fprint "Hello, my name is %s and I am %d years old.\n"
              (property this name) (property this age)]
        )
    ]
]
```

**Using the property:**

To access class fields, the property construct is used. This encapsulates access to variables and supports encapsulation principles.

Example of creating an object and calling a method:

```
[var (alice (newobj Person "Alice" 30))] // Create an instance of the Person class
[call (property alice greet)]               // Call the greet method on the alice instance
```

> Note: Ensure that you have a clear separation between application logic and data presentation. This improves the structure of the code.

## 3. Number Systems

Morning supports various number systems, allowing you to work with numbers in a convenient format:

- Hexadecimal system: use the prefix 0x (e.g., 0x1A).
- Binary system: use the prefix 0b (e.g., 0b1010).
- Octal system: use the prefix 0 (e.g., 012).

**Example of all number systems:**

```
[var (hexValue !int) 0x1A] // 26 in decimal
[var (binValue !int) 0b1010] // 10 in decimal
[var (octValue !int) 012] // 10 in decimal
```

> Note: Ensure you are using the correct format for representing numbers depending on the task to avoid confusion.

## 4. Code Style Recommendations

Following style recommendations improves the readability and maintainability of your code and helps other programmers adapt more quickly.

### 4.1. Naming Style

- Use lower_case for function names (e.g., calculate_area).
- Apply PascalCase for class names (e.g., Person).
- Write constants in UPPER_CASE (e.g., MAX_COUNT).
- Use camelCase for class methods (e.g., getName).

**Example of naming:**

```
[class Circle self
    [scope
        (var radius !frac)

        (func init (this r)
            [set (property this radius) r]
        )

        (func circleArea (this) -> !frac
            [return (* 3.14 (property this radius) (property this radius))]
        )
    ]
]
```

The init function is the class constructor and is required.

### 4.2. Indentation and Formatting

- Use 2 or 4 spaces for indentation, avoiding mixing spaces and tabs.
- Maintain uniformity in indentation and code structure to enhance readability.

### 4.3. Comments

- Always add comments to complex sections of code and describe the logic of functions.
- This not only helps you but also facilitates the work of other developers who may read and maintain your code.

Example of comments:

```
// Function to calculate the area of a circle
[func calculate_area ((circle Circle)) -> !frac
    [return (* 3.14 (property circle radius) (property circle radius))]
]
```

### 4.4. Code Cleanliness

- Break large functions into smaller ones to make the code more modular and understandable.
- Remove unused variables, functions, and code snippets to minimize confusion.
- Regularly refactor to improve structure and reduce code duplication.

> Note: Regularly evaluate your code in terms of simplicity and ease of use; this will help maintain high quality.

## Conclusion

By following these guidelines and exploring the capabilities of the Morning language, you will be able to create efficient, understandable, and maintainable code. Morning offers flexible structures for both functional and object-oriented programming. Don’t hesitate to experiment, share experiences, and actively engage in the developer community. Happy coding!

