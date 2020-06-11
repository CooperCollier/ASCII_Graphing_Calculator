#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdlib.h>

#define XLEFT -86
#define XRIGHT 86
#define YBOTTOM -36
#define YTOP 36

/** The ASCII Graphing Calculator : Implemented in C */
/** Author: Cooper Collier, June 2020                */
/** ------------------------------------------------ */



/** This variable is used in the user-interactive control flow. It determines whether
 * the user has entered a valid input, and therefore the program should advance. At most
 * steps in the 'main' method, the program only advances if this variable is true. Otherwise,
 * it repeats the last step (or returns to the beginning) until okay is true.*/
bool okay;

/** These variables are used to control the placement and zoom of the graph. The user can
 * adjust them after generating a graph. If the user generates a new graph, these variables
 * are reset to the default values of 0, 0, and 1.*/
int x_pan;
int y_pan;
float scale;

/** This is a binary tree of instructions, which stores the equation that will be
 * graphed. Each 'value' is an operation (*, + , -, etc) to perform on
 * the left and right branches--unless the node is a leaf node, in which case
 * the 'value' is either x or a number. This tree works similarly to a Lisp
 * program since each node contains an operator (value) and the pointers to
 * two operands.*/
struct instructionTree {
    struct instructionTree *left;
    struct instructionTree *right;
    char value[10];
};

/** This is a debugging function that prints out the contents
 * of a specified instructionTree. It is not required
 * for the main program implementation. */
void printTree(struct instructionTree *treePtr) {
    struct instructionTree tree = *treePtr;
    printf("VALUE: %s\n", (tree.value));
    if (tree.left == NULL && tree.right == NULL) {
        return;
    } else if (tree.left == NULL) {
        printf(" RIGHT: { \n");
        printTree(tree.right);
        printf(" } \n");
    } else if (tree.right == NULL) {
        printf(" LEFT: { \n");
        printTree(tree.left);
        printf(" } \n");
    } else {
        printf(" RIGHT: { \n");
        printTree(tree.right);
        printf(" } \n");
        printf(" LEFT: { \n");
        printTree(tree.left);
        printf(" } \n");
    }
}

/** This function takes in an 'in' string and an 'out' string. It copies
 * LENGTH elements from the in to the out, starting at position START
 * in the 'in' string. For some reason, using strcpy gave me weird
 * memory problems (overwriting random things), so I didn't use it. */
void copy (char out[], char in[], int start, int length) {
    for (int i = start, j = 0; j < length; i++, j++) {
        out[j] = in[i];
    }
    out[length] = '\0';
}

/** This function changes expressions like (x + 3) into
 * expressions like x + 3. It only strips the outermost pair
 * of parentheses, so ((x * 7)) would become (x * 7).
 * But it only applies to outermost parentheses--so "(x^(2)) + (ln((x)))"
 * would stay unchanged. There is also a 'loops' counter because this
 * function is recursive, so it tries to prevent infinite recursion.
 * (just in case) */
char* stripParenthesis(char string[], int size, int loops) {
    /** Part 1: In the base, case, if there are no parentheses we just return.
     * And if we have recursed more than 20 times, we return with an error. */
    if (string[0] != '(') {
        return string;
    } else if (loops > 20) {
        printf("(error 1) The equation appears to be incorrectly formatted. "
               "Please double-check what you typed in, or type \"i\" to see the formatting guide.\n");
        okay = false;
        return NULL;
    }
    /** Part 2: We go through the string, counting how many parentheses
     * we are 'inside' now. If we are ever 'inside' zero parentheses, we don't
     * need to strip anything, so we just return. For example, if we go through
     * ((x) + 3) * (log(sin(x))), upon hitting the "*" we are inside zero parentheses
     * so we can just exit. */
    int numParentheses = 1;
    int i = 1;
    while (numParentheses != 0 && i <= (size - 2)) {
        char letter = string[i];
        if (letter == '(') {
            numParentheses++;
        } else if (letter == ')') {
            numParentheses--;
        }
        i++;
    }
    /** If i == size - 1, that means we never exited the loop prematurely. So we
     * can go ahead and strip off the first and final parentheses. Then we
     * make a recursive call just in case the user formatted their input
     * as something ridiculous like this: (((((x + 7))))).  */
    if (i == size - 1) {
        char stripped[size - 2];
        copy(stripped, string,1, size - 1);
        return stripParenthesis(stripped, size - 2, loops + 1);
    } else {
        return string;
    }
}

/** A minus sign can represent two things: subtraction, as in (x - 3), and
 * negation, as in (3 * -x). This function takes in an equation and
 * the index of the minus sign in quesion. If the minus sign is for
 * subtraction, it returns true. If it is for negation, it returns false.
 * This function is used in part 3 of parseMath. */
bool isSubtract(char equation[], int index) {
    if (index == 0) {
        return false;
    }
    switch (equation[index - 1]) {
        case 'x':
            return true;
        case 'i':
            return true;
        case 'e':
            return true;
        case ')':
            return true;
        default:
            if (48 <= equation[index - 1] && 57 >= equation[index - 1]) {
                return true;
            } else {
                return false;
            }
    }
}

/** This function takes in an equation and generates a respective
 * instructionTree. It works recursively for each level of the tree. There
 * is also a 'loops' variable because this function is recursive--it is
 * incrememnted with every recursive call, and in the event of an infinite
 * loop it will get too big and throw an error. */
struct instructionTree *parseMath(char equation[], int size, int loops) {
    /** Part 1: Instantiate important variables; leftPtr will point
     * to leftHalf and leftSize is the size of leftHalf. leftPtr will
     * be called in the recursive call. Same deal for rightHalf. Value
     * is the operand. So if equation = "(x^2) * (3 - cosx)", then:
     * value = "*"
     * leftHalf = "(x^2)"
     * rightHalf = "(3 * cosx)"
     * We also use stripParentheses on the equation to clean it up.
     * And we re-calculate the size of the equation, because stripping
     * parentheses can shorten it. */
     if (loops > 50) {
         printf("(error 2) The equation appears to be incorrectly formatted. "
                "Please double-check what you typed in, or type \"i\" to see the formatting guide.\n");
         okay = false;
         return NULL;
     }
    int leftSize;
    int rightSize;
    char leftHalf[100];
    char rightHalf[100];
    struct instructionTree *leftPtr = NULL;
    struct instructionTree *rightPtr = NULL;
    char value[10];
    equation = stripParenthesis(equation, size, 0);
    if (equation == NULL) {
        return NULL;
    }
    size = 0;
    for (int i = 0; i < 100; i++) {
        if (equation[i] == '\0') {
            size = i;
            break;
        }
    }
    /** Part 2: Try searching for two-arguments operations. We iterate through the
     * instructions array, and for each instruction, we iterate through the equation and
     * check if that instruction is present--the array is
     * organized according to the order of operations (PEMDAS).
     * We only consider instructions that are not contained in parentheses, so we
     * must count how many left & right parentheses we have seen so far. */
    char instructions[] = {'-', '+', '/', '*', '^'};
    bool operationFound = false;
    for (int i = 0; i < sizeof instructions && !operationFound; i++) {
        char instruction = instructions[i];
        int numParentheses = 0;
        for (int j = 0; j < size && !operationFound; j++) {
            if (equation[j] == '(') {
                numParentheses ++;
            } else if (equation[j] == ')') {
                numParentheses --;
            } else if (equation[j] == instruction && numParentheses == 0) {
                /** Part 3: Once we find a two-argument operation, we split
                 * the equation around that operation. LeftHalf is everything before
                 * the operation, RightHalf is everything after it. Value is the operation
                 * itself. We also set operationFound to true so we can exit out of the
                 * two for loops. */
                if (instruction == '-') {
                    if (!isSubtract(equation, j)) {
                        continue;
                    }
                }
                copy(leftHalf, equation, 0, j);
                copy(rightHalf, equation, j + 1, size - (j + 1));
                leftSize = j;
                rightSize = size - (leftSize + 1);
                value[0] = equation[j];
                value[1] = '\0';
                operationFound = true;
            }
        }
    }
    /** Part 4: The recursive calls. We create leftPtr and rightPtr. */
    if (operationFound) {
        leftPtr = parseMath(leftHalf, leftSize, loops + 1);
        rightPtr = parseMath(rightHalf, rightSize, loops + 1);
    } else {
        /** Part 5: This is where we go if there were no 2-argument operands
         * found. In here, we look for one-element operands like sin & cos. This
         * is also where we handle the two base cases--equation is either just x,
         * or just a number. We also check for the constants e and pi. The numbers
         * 48 and 57 appear below because they are the limits for ACII integers 0-9.*/
        switch (equation[0]) {
            case '-':
                copy(value, "neg", 0, 3);
                copy(rightHalf, equation, 1, size - 1);
                rightPtr = parseMath(rightHalf, size - 1, loops + 1);
                break;
            case 's':
                copy(value, "sin", 0, 3);
                copy(rightHalf, equation, 3, size - 3);
                rightPtr = parseMath(rightHalf, size - 3, loops + 1);
                break;
            case 'c':
                copy(value, "cos", 0, 3);
                copy(rightHalf, equation, 3, size - 3);
                rightPtr = parseMath(rightHalf, size - 3, loops + 1);
                break;
            case 't':
                copy(value, "tan", 0, 3);
                copy(rightHalf, equation, 3, size - 3);
                rightPtr = parseMath(rightHalf, size - 3, loops + 1);
                break;
            case 'l':
                if (equation[1] == 'o') {
                    copy(value, "log", 0, 3);
                    copy(rightHalf, equation, 3, size - 3);
                    rightPtr = parseMath(rightHalf, size - 3, loops + 1);
                    break;
                } else if (equation[1] == 'n') {
                    copy(value, "ln", 0, 2);
                    copy(rightHalf, equation, 2, size - 2);
                    rightPtr = parseMath(rightHalf, size - 2, loops + 1);
                    break;
                }
            case 'x':
                copy(value, "x", 0, 1);
                break;
            case 'e':
                copy(value, "2.7183", 0, 6);
                break;
            case 'p':
                copy(value, "3.1416", 0, 6);
                break;
            default:
                if (48 <= equation[0] && 57 >= equation[0]) {
                    copy(value, equation, 0, 9);
                } else {
                    printf("(error 3) The equation appears to be incorrectly formatted. "
                           "Please double-check what you typed in, or type \"i\" to see the formatting guide.\n");
                    okay = false;
                    return NULL;
                }
        }
    }
    /** Part 6: Finally, we can build the instructionTree and return the pointer to it. */
    struct instructionTree *treePtr = malloc(200);
    struct instructionTree tree = {leftPtr, rightPtr, "Nothing"};
    *treePtr = tree;
    copy(treePtr->value, value, 0, 10);
    return treePtr;
}

/** This function takes an operation and two floats, and applies the
 * operation to them, returning the result. Some operations (like sin)
 * only need one operand, so in that case the other operand will enter
 * the function set to zero. */
float doMath(char operation[], float x1, float x2) {
    if (strcmp(operation, "+") == 0) {
        return x1 + x2;
    } else if (strcmp(operation, "-") == 0) {
        return x1 - x2;
    } else if (strcmp(operation, "*") == 0) {
        return x1 * x2;
    } else if (strcmp(operation, "/") == 0) {
        return x1 / x2;
    } else if (strcmp(operation, "^") == 0) {
        return pow(x1, x2);
    } else if (strcmp(operation, "neg") == 0) {
        if (x1 == 0) {
            return (-1 * x2);
        } else {
            return (-1 * x1);
        }
    } else if (strcmp(operation, "sin") == 0) {
        if (x1 == 0) {
            return sin(x2);
        } else {
            return sin(x1);
        }
    } else if (strcmp(operation, "cos") == 0) {
        if (x1 == 0) {
            return cos(x2);
        } else {
            return cos(x1);
        }
    } else if (strcmp(operation, "tan") == 0) {
        if (x1 == 0) {
            return tan(x2);
        } else {
            return tan(x1);
        }
    } else if (strcmp(operation, "log") == 0) {
        if (x1 == 0) {
            return log10(x2);
        } else {
            return log10(x1);
        }
    } else if (strcmp(operation, "ln") == 0) {
        if (x1 == 0) {
            return log(x2);
        } else {
            return log(x1);
        }
    } else {
        printf("(error 4) The equation appears to be incorrectly formatted. "
               "Please double-check what you typed in, or type \"i\" to see the formatting guide.\n");
        okay = false;
        return 0;
    }
}

/** This function takes in a value for x, and mechanically runs it through
 * the instructionTree in a recursive fashion, calling doMath() each time.
 * Anytime "x" appears as a value in the tree, it is substituted with the current value of x. */
float calculate(float x, struct instructionTree *treePtr) {
    struct instructionTree tree = *treePtr;
    if (tree.left == NULL && tree.right == NULL) {
        if (strcmp(tree.value, "x") == 0) {
            return x;
        } else {
            return atof((tree.value));
        }
    } else if (tree.left == NULL) {
        return doMath((tree.value), 0, calculate(x, tree.right));
    } else if (tree.right == NULL) {
        return doMath((tree.value), calculate(x, tree.left), 0);
    } else {
        return doMath((tree.value), calculate(x, tree.left), calculate(x, tree.right));
    }
}

/** This function visualizes the graph by printing out every square. This function calls
 * the calculate function on x for every (x, y) pair, and only fills in a square if
 * calculate(x) == y. This functions also adds some asthetic borders and marks the coordinates of
 * the four corners. This function also scales and pans the graph according to the
 * global variables x_pan, y_pan and scale.*/
void printGraph(int left, int right, int bottom, int top, struct instructionTree *treePtr) {
    /** Part 1: Verify that all input is correct, and declare important variables. */
    if (left > right || bottom > top) {
        printf("(error 5) There was an error drawing the graph because the specified dimensions were incorrect.\n");
        okay = false;
        return;
    }
    /** Part 2: Print out the corner coordinates, and some asthetic borders.*/
    printf("\n");
    printf("Top left [X:%f, Y:%f], ", ((left * scale) + x_pan), ((top * scale) + y_pan));
    printf("Top right [X:%f, Y:%f], ", ((right * scale) + x_pan), ((top * scale) + y_pan));
    printf("Bottom left [X:%f, Y:%f], ", ((left * scale) + x_pan), ((bottom * scale) + y_pan));
    printf("Bottom right [X:%f, Y:%f]\n", ((right * scale) + x_pan), ((bottom * scale) + y_pan));
    for (int i = 0; i < ((right - left) / 2); i++) {
        printf("[]");
    }
    printf("\n");
    /** Part 3: Actually print out the contents of the graph.*/
    for (int y = top; y >= bottom; y--) {
        for (int x = left; x <= right; x++) {
            int this_x = x;
            this_x *= scale;
            this_x -= x_pan;
            float result1 = calculate(this_x, treePtr);
            int result = round(result1);
            result -= y_pan;
            if (result == y) {
                printf("*");
            } else if (x == 0) {
                printf("|");
            } else if (y == 0) {
                printf("_");
            } else {
                printf(" ");
            }
        }
        printf("\n");
    }
    /** Part 4: Print out some more asthetic borders.*/
    for (int i = 0; i < ((right - left) / 2); i++) {
        printf("[]");
    }
    printf("\n\n");
}

/** This function alters the 'scale' global variable and prints out
 * a new graph. The numbers 48 and 57 appear because they correspond
 * to the bounds for ASCII integers 0-9.
 * The input to this function should be in the form "z n", where n
 * is a float or an int. The variable 'decimals' appears because
 * every character in contents[] must be a number, except for one
 * allowed decimal point somewhere in the contents[] array. The 'first'
 * variable appears because it isn't acceptable for a decimal to be
 * the first digit.*/
void zoom(char contents[], struct instructionTree *treePtr) {
    contents += 2;
    char fullContents[30];
    copy(fullContents, contents, 0, 30);
    int decimals = 0;
    bool first = true;
    for (; contents[0] != '\0'; contents++) {
        if (48 > contents[0] || 57 < contents[0]) {
            if (decimals < 1 && contents[0] == '.' && !first) {
                decimals++;
                continue;
            } else {
                printf("Arguments to 'zoom' were not in the format:"
                       " \"z n\" where n is a number or a decimal.\n");
                return;
            }
        }
        first = false;
    }
    scale *= atof(fullContents);
    printGraph(XLEFT, XRIGHT, YBOTTOM, YTOP, treePtr);
}

/** This function alters the 'x_pan' global variable and prints out
 * a new graph. The numbers 48 and 57 appear because they correspond
 * to the bounds for ASCII integers 0-9. The input to this function
 * should be in the form "x n", where n is a positive or negative int.
 * The 'first' variable appears because every character in contents[]
 * must be a number, except for the first one, which can potentially
 * be a minus sign.*/
void xPan(char contents[], struct instructionTree *treePtr) {
    contents += 2;
    char fullContents[30];
    copy(fullContents, contents, 0, 30);
    bool first = true;
    for (; contents[0] != '\0'; contents++) {
        if (48 > contents[0] || 57 < contents[0]) {
            if (first && contents[0] == '-') {
                continue;
            } else {
                printf("Arguments to 'x pan' were not in the format:"
                       " \"x n\" where n is a positive or negative number.\n");
                return;
            }
        }
        first = false;
    }
    x_pan += atoi(fullContents);
    printGraph(XLEFT, XRIGHT, YBOTTOM, YTOP, treePtr);
}

/** This function alters the 'y_pan' global variable and prints out
 * a new graph. The numbers 48 and 57 appear because they correspond
 * to the bounds for ASCII integers 0-9. The input to this function
 * should be in the form "y n", where n is a positive or negative int.
 * The 'first' variable appears because every character in contents[]
 * must be a number, except for the first one, which can potentially
 * be a minus sign.*/
void yPan(char contents[], struct instructionTree *treePtr) {
    contents += 2;
    char fullContents[30];
    copy(fullContents, contents, 0, 30);
    bool first = true;
    for (; contents[0] != '\0'; contents++) {
        if (48 > contents[0] || 57 < contents[0]) {
            if (first && contents[0] == '-') {
                continue;
            } else {
                printf("Arguments to 'y pan' were not in the format:"
                       " \"y n\" where n is a positive or negative number.\n");
                return;
            }
        }
        first = false;
    }
    y_pan -= atoi(fullContents);
    printGraph(XLEFT, XRIGHT, YBOTTOM, YTOP, treePtr);
}

/** Prints out instructions for how to use this graphing calculator. */
void printInstructions() {
    printf("-------------------------------------------------------------------------------\n"
           "How to use this graphing calculator:\n"
           "At any time, you can type 'q' to quit, or type 'i' to read these instructions.\n"
           "You can also type in an equation to graph. This calculator supports the \n"
           "following operands: +, -, *, /, ^, sin, cos, tan, log, ln, e, pi, and any real\n"
           "number. All equations should be formatted in terms of x. Note that\n"
           "multiplication written like \"5x\" is not supported; multiplication must be\n"
           "done explicitly, like \"5 * x\". Also, it is recommended to use parentheses to\n"
           "clarify the order of operations for your equation. Once you have graphed an\n"
           "equation, you can pan n units in the x-direction by typing \"x n\", or pan\n"
           "n units in the y-direction by typing \"y n\", or zoom by a factor of n by \n"
           "typing \"z n\". For zooming, if n is less than one then the graph will zoom in.\n\n"
           "Here are some examples of correctly formatted equations:\n"
           "((sin x) * 3) ^ 1.5\n"
           "27 - ((x / -cosx) + (-2) * (log x))\n"
           "(5 - lnx) * -sin(x)\n"
           "4 * x^ -2 - 2 * tanx / ln x ^3\n"
           "((((8 - ln x)) * e)) + 0.85\n\n"
           "A final note: Since ASCII text is taller than it is wide, the graph output\n"
           "may appear stretched vertically.\n"
           "-------------------------------------------------------------------------------\n");
}

int main() {
    printf("Welcome to the ASCII graphing calculator!\n");
    printf("For instructions on how to use this calculator, type \"i\" at any time.\n");
    printf("To quit, type \"q\" at any time.\n");
    printf("-------------------------------------------------------------------------------\n\n");

    /** This loops runs for as long as the user has not pressed 'q' to quit. Each time
     * the loop runs, the user goes through the process of generating a new graph. */
    while (true) {
        /** Part 1: set all important variables to default values. The contents of equation,
         * cleanEquation and input are all set to null. Then the user is prompted to input
         * either an equation, the 'i' command or the 'q' command. The size variable is computed,
         * which keeps track of the equation's size. The input is stored in the "input" array, and if it
         * is not 'q' or 'i' then it gets copied to the "equation" array.*/
        okay = false;
        x_pan = 0;
        y_pan  = 0;
        scale = 1;
        char equation[100];
        char cleanEquation[100];
        char input[100];
        for (int i = 0; i < 100; i++) {
            equation[i] = '\0';
            cleanEquation[i] = '\0';
            input[i] = '\0';
        }
        int size = 0;
        printf("Enter the equation you want to graph:\n");
        scanf("%[^\n]%*c", input);
        if (strcmp(input, "q") == 0 || strcmp(input, "Q") == 0) {
            printf("Exiting... \n");
            return 0;
        } else if (strcmp(input, "i") == 0 || strcmp(input, "I") == 0) {
            printInstructions();
        } else {
            okay = true;
            for (int i = 0; i < 100; i++) {
                if (input[i] == '\0') {
                    size = i;
                    break;
                }
            }
            copy(equation, input, 0, size);
        }
        /** Part 2: Count the number of open- and close-parentheses in the equation, and
         * throw an error if they are not equal.*/
        if (okay) {
            int openParens = 0;
            int closeParens = 0;
            for (int i = 0; i < size; i++) {
                if (equation[i] == ')') {
                    closeParens += 1;
                } else if (equation[i] == '(') {
                    openParens += 1;
                }
            }
            if (openParens != closeParens) {
                printf("(error 6) The equation appears to be incorrectly formatted. "
                       "Please make sure you have the same number of open & close parentheses.\n");
                okay = false;
            }
        }
        /** Part 3: Clean up the equation by converting everything to lowercase
         * and removing all whitespace. Store the result in cleanEquation. 'Removed'
         * is the number of whitespace characters we remove in this process. */
        int removed = 0;
        if (okay) {
            for (int i = 0, j = 0; i < size; i++) {
                if (equation[i] != ' ') {
                    char clean = tolower(equation[i]);
                    cleanEquation[j] = clean;
                    j++;
                } else {
                    removed += 1;
                }
            }
        }
        /** Part 4: Basically all of the important stuff happens here. The instructionTree is
         * created, and then it is used to call printGraph. The locations at the graph corners
         * are printed out for asthetic purposes. */
        struct instructionTree *treePtr;
        if (okay) {
            treePtr = parseMath(cleanEquation, (size) - removed, 0);
        }
        if (okay) {
            printGraph(XLEFT, XRIGHT, YBOTTOM, YTOP, treePtr);
        }
        /** Part 5: A section that allows the user to pan and zoom around the graph for
         * however long they like. Each call to one of those functions prints a new
         * graph, with the zoom & pan modified.
         * A while loop keeps running that only exits if they enter a command other than zoom, pan, or i. */
        if (okay) {
            bool zooming = true;
            while (zooming) {
                printf("To pan the graph n units in the x direction, type \"x n\"."
                       " To pan the graph n units in the y direction, type \"y n\". \n");
                printf("To zoom the graph by a factor of n, type \"z n\","
                       " where n is a decimal of you are zooming in. \n");
                printf("To draw a new graph, type anything else.\n");
                scanf("%[^\n]%*c", input);
                if (input[0] == 'x' && input[1] == ' ') {
                    xPan(input, treePtr);
                } else if (input[0] == 'y' && input[1] == ' ') {
                    yPan(input, treePtr);
                } else if (input[0] == 'z' && input[1] == ' ') {
                    zoom(input, treePtr);
                } else if (strcmp(input, "i") == 0 || (strcmp(input, "I") == 0)) {
                    printInstructions();
                } else if (strcmp(input, "q") == 0 || (strcmp(input, "Q") == 0)) {
                    printf("Exiting... \n");
                    return 0;
                } else {
                    if (strcmp(input, "anything else") == 0) {
                        printf("Yeah, very funny.\n");
                    }
                    zooming = false;
                }
            }
        }
    }
}