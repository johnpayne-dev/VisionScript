#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "Parser.h"

static SyntaxError ParseExpression(list(Token) tokens, int32_t start, int32_t end, Expression ** expression);

const char * SyntaxErrorToString(SyntaxErrorCode code)
{
	switch (code)
	{
		case SyntaxErrorNone: return "no error";
		case SyntaxErrorUnknownToken: return "unknown token";
		case SyntaxErrorInvalidVariableDeclaration: return "invalid variable declaration";
		case SyntaxErrorInvalidFunctionDeclaration: return "invalid function declaration";
		case SyntaxErrorMissingExpression: return "missing expression";
		case SyntaxErrorMissingBracket: return "missing corresponding bracket";
		case SyntaxErrorNonsenseExpression: return "unable to understand expression";
		case SyntaxErrorMissingOperon: return "missing operon";
		case SyntaxErrorTooManyVectorElements: return "too many elements in vector initialization";
		case SyntaxErrorInvalidCommaPlacement: return "unexpected comma placement";
		case SyntaxErrorInvalidForPlacement: return "for operator can only be placed in array";
		case SyntaxErrorInvalidForAssignment: return "invalid for assignment";
		case SyntaxErrorInvalidForAssignmentPlacement: return "for assignment can only be used within for operator";
		case SyntaxErrorInvalidEllipsisPlacement: return "ellipsis can only be used within an array";
		case SyntaxErrorInvalidEllipsisOperon: return "invalid ellipsis operon";
		case SyntaxErrorIndexingWithVector: return "cannot index an array with a vector";
		case SyntaxErrorInvalidFunctionCall: return "invalid function call syntax";
		default: return "unknown error";
	}
}

static int32_t FindCorrespondingBracket(list(Token) tokens, int32_t start, int32_t end, int32_t bracketIndex)
{
	// determine which bracket to look for (and the corresponding direction)
	bool findLeft = false;
	char bracket = '\0';
	switch (tokens[bracketIndex].value[0])
	{
		case '(': bracket = ')'; findLeft = false; break;
		case ')': bracket = '('; findLeft = true; break;
		case '{': bracket = '}'; findLeft = false; break;
		case '}': bracket = '{'; findLeft = true; break;
		case '[': bracket = ']'; findLeft = false; break;
		case ']': bracket = '['; findLeft = true; break;
		default: return -1;
	}
	
	// search for the bracket; non-recursive by keeping track of the bracket 'depth'
	int32_t index = -1;
	int32_t depth = 0;
	int32_t searchStart = findLeft ? bracketIndex - 1 : bracketIndex + 1;
	int32_t breakCondition = findLeft ? start - 1 : end + 1;
	for (int32_t i = searchStart; i != breakCondition; i += findLeft ? -1 : 1)
	{
		if (tokens[i].value[0] == tokens[bracketIndex].value[0]) { depth++; }
		if (tokens[i].value[0] == bracket)
		{
			if (depth == 0) { index = i; break; }
			else { depth--; }
		}
	}
	return index;
}

static StatementType DetermineStatementType(list(Token) tokens)
{
	// if there's zero or one tokens then a statement declaration isn't possible
	if (ListLength(tokens) < 2) { return StatementTypeUnknown; }
	
	// variable and function declarations must start with an identifier
	if (tokens[0].type == TokenTypeIdentifier)
	{
		// if second token is a '(' then it must be a function
		if (tokens[1].type == TokenTypeBracket && tokens[1].value[0] == '(')
		{
			// check to make sure the corresponding ')' and '=' exist
			int32_t index = FindCorrespondingBracket(tokens, 1, ListLength(tokens) - 1, 1);
			if (index > -1 && index != ListLength(tokens) - 1 && tokens[index + 1].value[0] == '=') { return StatementTypeFunction; }
		}
		// if second token is a '=' then it must be a variable
		if (tokens[1].type == TokenTypeSymbol && tokens[1].value[0] == '=') { return StatementTypeVariable; }
		return StatementTypeUnknown;
	}
	
	// render declaration must start with a keyword
	if (tokens[0].type == TokenTypeKeyword)
	{
		if (StringEquals(tokens[0].value, "point") || StringEquals(tokens[0].value, "parametric") || StringEquals(tokens[0].value, "polygon"))
		{
			return StatementTypeRender;
		}
	}
	return StatementTypeUnknown;
}

static SyntaxError ParseVariableDeclaration(list(Token) tokens, StatementDeclaration * declaration, int32_t * declarationEndIndex)
{
	// since DetermineStatementType checks if there's an identifier and '=', no need to do that here
	declaration->variable.identifier = StringCreate(tokens[0].value);
	*declarationEndIndex = 1;
	return (SyntaxError){ SyntaxErrorNone };
}

static SyntaxError ParseFunctionDeclaration(list(Token) tokens, StatementDeclaration * declaration, int32_t * declarationEndIndex)
{
	// get the index of the ')', no need to check for '=' since that's ensured by DetermineStatementType
	int32_t argumentsEndIndex = FindCorrespondingBracket(tokens, 1, ListLength(tokens) - 1, 1);
	
	// go through each identifier and add it to the argument list
	declaration->function.arguments = ListCreate(sizeof(String), 4);
	for (int32_t i = 2; i < argumentsEndIndex; i++)
	{
		if (tokens[i].type != TokenTypeIdentifier) { return (SyntaxError){ SyntaxErrorInvalidFunctionDeclaration, i, i }; }
		ListPush((void **)&declaration->function.arguments, &(String){ StringCreate(tokens[i].value) });
		i++;
		
		if (i == argumentsEndIndex) { break; }
		if (tokens[i].type != TokenTypeSymbol || tokens[i].value[0] != ',') { return (SyntaxError){ SyntaxErrorInvalidFunctionDeclaration, i, i }; }
	}
	declaration->function.identifier = StringCreate(tokens[0].value);
	*declarationEndIndex = argumentsEndIndex + 1;
	return (SyntaxError){ SyntaxErrorNone };
}

static SyntaxError ParseRenderDeclaration(list(Token) tokens, StatementDeclaration * declaration, int32_t * declarationEndIndex)
{
	if (StringEquals(tokens[0].value, "point")) { declaration->render.type = StatementRenderTypePoint; }
	if (StringEquals(tokens[0].value, "parametric")) { declaration->render.type = StatementRenderTypeParametric; }
	if (StringEquals(tokens[0].value, "polygon")) { declaration->render.type = StatementRenderTypePolygon; }
	*declarationEndIndex = 0;
	return (SyntaxError){ SyntaxErrorNone };
}

static int32_t FindComma(list(Token) tokens, int32_t start, int32_t end)
{
	// finds a comma in the same bracket 'depth' that the search starts in
	int32_t depth = 0;
	for (int32_t i = start; i <= end; i++)
	{
		if (tokens[i].type == TokenTypeBracket)
		{
			if (tokens[i].value[0] == '(' || tokens[i].value[0] == '[' || tokens[i].value[0] == '{') { depth++; }
			if (tokens[i].value[0] == ')' || tokens[i].value[0] == ']' || tokens[i].value[0] == '}') { depth--; }
			if (depth < 0) { return -1; } // if depth is ever negative then there's no comma within the initial bracket depth
		}
		if (depth == 0 && tokens[i].type == TokenTypeSymbol && tokens[i].value[0] == ',') { return i; }
	}
	return -1;
}

static SyntaxError FindOperator(list(Token) tokens, int32_t start, int32_t end, const char * operators[], int32_t operatorCount, bool rightToLeft, int32_t * index)
{
	*index = -1;
	// sets up to search right-to-left or left-to-right
	int32_t i = rightToLeft ? end : start;
	while (rightToLeft ? (i >= start) : (i <= end))
	{
		// if search hits a bracket, skip over its contents
		if (tokens[i].type == TokenTypeBracket)
		{
			int32_t bracketIndex = FindCorrespondingBracket(tokens, start, end, i);
			if (bracketIndex < 0) { return (SyntaxError){ SyntaxErrorMissingBracket, i, end }; }
			i = bracketIndex + (rightToLeft ? -1 : 1);
			continue;
		}
		
		if (tokens[i].type == TokenTypeOperator)
		{
			// compare against each operator passed through this function to see if there's a match
			bool found = false;
			for (int32_t j = 0; j < operatorCount; j++)
			{
				const char * operator = operators[j];
				// 'B', short for binary, means that the operator has a corresponding unary version that should be taken into account when searching
				if (operator[0] == 'B')
				{
					// 'L', short for left, means that the corresponding unary operator is placed to the left of the operon
					if (operator[1] == 'L' && (i == start || tokens[i - 1].type == TokenTypeOperator)) { continue; }
					// 'R', short for right, means that the corresponding unary operator is placed to the right of the operon
					if (operator[1] == 'R' && (i == end || tokens[i + 1].type == TokenTypeOperator)) { continue; }
					operator += 2;
				}
				// 'U', short for unary, means that it's a unary operator
				else if (operator[0] == 'U')
				{
					// 'L', short for left, means that operator should be placed to the left of the operon
					if (operator[1] == 'L' && i != start) { continue; }
					// 'R', short for right, means that operator should be placed to the right of the operon
					if (operator[1] == 'R' && i != end) { continue; }
					operator += 2;
				}
				if (StringEquals(tokens[i].value, operator)) { found = true; break; }
			}
			if (found) { *index = i; break; }
		}
		i += rightToLeft ? -1 : 1;
	}
	return (SyntaxError){ SyntaxErrorNone };
}

static SyntaxError FindFunctionCall(list(Token) tokens, int32_t start, int32_t end, int32_t * index)
{
	*index = -1;
	// searches for a function call between start to end
	int32_t i = start;
	while (i <= end)
	{
		// since function calls have highest precedence, if search runs into a '(' then assume it's a function call
		if (tokens[i].type == TokenTypeBracket)
		{
			// check to make sure there's an identifier and the parentheses are closed
			if (tokens[i].value[0] == '(' && i > 0 && tokens[i - 1].type == TokenTypeIdentifier) { *index = i; }
			int32_t bracketIndex = FindCorrespondingBracket(tokens, start, end, i);
			if (bracketIndex < 0) { return (SyntaxError){ SyntaxErrorMissingBracket, i, end }; }
			i = bracketIndex + 1;
			continue;
		}
		i++;
	}
	return (SyntaxError){ SyntaxErrorNone };
}

static OperonType DetermineOperonType(list(Token) tokens, int32_t start, int32_t end)
{
	// if this is returned then SyntaxErrorMissingOperon should be thrown
	if (start > end) { return OperonTypeNone; }
	// vector literal must have () and at least one comma between them
	if (tokens[start].value[0] == '(' && FindCorrespondingBracket(tokens, start, end, start) == end && FindComma(tokens, start + 1, end - 1) >= 0) { return OperonTypeVectorLiteral; }
	// array literals must have []
	if (tokens[start].value[0] == '[' && FindCorrespondingBracket(tokens, start, end, start) == end) { return OperonTypeArrayLiteral; }
	// identifier must be one token long
	if (start == end && tokens[start].type == TokenTypeIdentifier) { return OperonTypeIdentifier; }
	// constant must be one token long
	if (start == end && tokens[start].type == TokenTypeNumber) { return OperonTypeConstant; }
	// for assignment must have an identifier with an '=' immediately after
	if (end - start >= 1 && tokens[start].type == TokenTypeIdentifier && tokens[start + 1].value[0] == '=') { return OperonTypeForAssignment; }
	// arguments must have at least one comma
	if (FindComma(tokens, start, end) >= 0) { return OperonTypeArguments; }
	// if it's not one of the above then assume it's an expression
	return OperonTypeExpression;
}

static SyntaxError ReadOperon(list(Token) tokens, int32_t start, int32_t end, OperonType type, Operon * operon)
{
	if (type == OperonTypeExpression) { return ParseExpression(tokens, start, end, &operon->expression); } // recursively call ParseExpression
	if (type == OperonTypeIdentifier)
	{
		operon->identifier = StringCreate(tokens[start].value);
		return (SyntaxError){ SyntaxErrorNone };
	}
	if (type == OperonTypeConstant)
	{
		operon->constant = atof(tokens[start].value);
		return (SyntaxError){ SyntaxErrorNone };
	}
	if (type == OperonTypeForAssignment)
	{
		operon->assignment.identifier = StringCreate(tokens[start].value);
		return ParseExpression(tokens, start + 2, end, &operon->assignment.expression);
	}
	// if operon type involves commas then add each expression into a list
	if (type == OperonTypeArguments || type == OperonTypeArrayLiteral || type == OperonTypeVectorLiteral)
	{
		operon->expressions = ListCreate(sizeof(Expression *), 4);
		// disregard start and end brackets
		if (type == OperonTypeArrayLiteral || type == OperonTypeVectorLiteral) { start += 1; end -= 1; }
		// if it's empty then return with the empty list (only used for OperonTypeArguments)
		if (start == end + 1) { return (SyntaxError){ SyntaxErrorNone }; }
		
		// iterate until there's no commas left
		int32_t prevStart = start;
		int32_t commaIndex = FindComma(tokens, start, end);
		int32_t elementIndex = 0;
		while (commaIndex > -1)
		{
			// return error if vector literal contains more than 4 elements
			if (type == OperonTypeVectorLiteral && elementIndex >= 3) { return (SyntaxError){ SyntaxErrorTooManyVectorElements, commaIndex, end }; }
			
			ListPush((void **)&operon->expressions, &(Expression *){ NULL });
			SyntaxError error = ParseExpression(tokens, prevStart, commaIndex - 1, &operon->expressions[elementIndex]);
			if (error.code != SyntaxErrorNone) { return error; }
			
			prevStart = commaIndex + 1;
			commaIndex = FindComma(tokens, commaIndex + 1, end);
			elementIndex++;
		}
		// add the last expression (since there's n-1 commas for n arguments)
		ListPush((void **)&operon->expressions, &(Expression *){ NULL });
		return ParseExpression(tokens, prevStart, end, &operon->expressions[elementIndex]);
	}
	return (SyntaxError){ SyntaxErrorNone };
}

static Operator OperatorType(String string, int32_t precedence)
{
	if (precedence == 0) { return OperatorNone; }
	if (StringEquals(string, "for")) { return OperatorFor; }
	if (StringEquals(string, "...")) { return OperatorEllipsis; }
	if (StringEquals(string, "+"))
	{
		if (precedence == 5) { return OperatorAdd; }
		else { return OperatorPositive; }
	}
	if (StringEquals(string, "-"))
	{
		if (precedence == 5) { return OperatorSubtract; }
		else { return OperatorNegative; }
	}
	if (StringEquals(string, "*")) { return OperatorMultiply; }
	if (StringEquals(string, "/")) { return OperatorDivide; }
	if (StringEquals(string, "%")) { return OperatorModulo; }
	if (StringEquals(string, "^")) { return OperatorPower; }
	if (StringEquals(string, ".")) { return OperatorIndex; }
	if (StringEquals(string, "(") && precedence == 1) { return OperatorFunctionCall; }
	return OperatorNone;
}

static bool IsUnaryOperator(Operator operator)
{
	return operator == OperatorPositive || operator == OperatorNegative;
}

static bool InsideArray(list(Token) tokens, int32_t index)
{
	// checks if a token at a given index is directly inside an [] (only 1 depth in, e.g. [(n)] would return false but [n] would return true)
	int32_t depth = 0;
	for (int32_t i = index; i <= ListLength(tokens); i++)
	{
		if (tokens[i].type == TokenTypeBracket)
		{
			if (depth == 0 && tokens[i].value[0] == ']') { return true; }
			if (tokens[i].value[0] == '(' || tokens[i].value[0] == '[' || tokens[i].value[0] == '{') { depth++; }
			if (tokens[i].value[0] == ')' || tokens[i].value[0] == ']' || tokens[i].value[0] == '}') { depth--; }
		}
	}
	return false;
}

static SyntaxError CheckOperatorLogic(Expression * expression, list(Token) tokens, int32_t opIndex)
{
	// if arguments are used anywhere but in function call, return an error
	if (expression->operonTypes[0] == OperonTypeArguments) { return (SyntaxError){ SyntaxErrorInvalidCommaPlacement, expression->operonStart[0], expression->operonEnd[0] }; }
	if (expression->operonTypes[1] == OperonTypeArguments && expression->operator != OperatorFunctionCall) { return (SyntaxError){ SyntaxErrorInvalidCommaPlacement, expression->operonStart[1], expression->operonEnd[1] }; }
	// if for assignment is used anywhere but in for operator, return an error
	if (expression->operonTypes[0] == OperonTypeForAssignment) { return (SyntaxError){ SyntaxErrorInvalidForAssignmentPlacement, expression->operonStart[0], expression->operonEnd[0] }; }
	if (expression->operonTypes[1] == OperonTypeForAssignment && expression->operator != OperatorFor) { return (SyntaxError){ SyntaxErrorInvalidForAssignmentPlacement, expression->operonStart[1], expression->operonEnd[1] }; }
	switch (expression->operator)
	{
		// return error if for operator is not inside an array or right hand operon is not a for assignment
		case OperatorFor:
			if (!InsideArray(tokens, opIndex)) { return (SyntaxError){ SyntaxErrorInvalidForPlacement, opIndex, opIndex }; }
			if (expression->operonTypes[1] != OperonTypeForAssignment) { return (SyntaxError){ SyntaxErrorInvalidForAssignment, expression->operonStart[1], expression->operonEnd[1] }; }
			break;
		// return error if ellipsis is not in an array or if either operon is not a scalar
		case OperatorEllipsis:
			if (!InsideArray(tokens, opIndex)) { return (SyntaxError){ SyntaxErrorInvalidEllipsisPlacement, opIndex, opIndex }; }
			if (expression->operonTypes[0] == OperonTypeArrayLiteral || expression->operonTypes[0] == OperonTypeVectorLiteral) { return (SyntaxError){ SyntaxErrorInvalidEllipsisOperon, expression->operonStart[0], expression->operonEnd[0] }; }
			if (expression->operonTypes[1] == OperonTypeArrayLiteral || expression->operonTypes[1] == OperonTypeVectorLiteral) { return (SyntaxError){ SyntaxErrorInvalidEllipsisOperon, expression->operonStart[1], expression->operonEnd[1] }; }
			break;
		// return error if trying to index with a vector literal
		case OperatorIndex:
			if (expression->operonTypes[1] == OperonTypeVectorLiteral) { return (SyntaxError){ SyntaxErrorIndexingWithVector, expression->operonStart[1], expression->operonEnd[1] }; }
			break;
		// return error if left operon isn't an identifier
		case OperatorFunctionCall:
			if (expression->operonTypes[0] != OperonTypeIdentifier) { return (SyntaxError){ SyntaxErrorInvalidFunctionCall, expression->operonStart[0], expression->operonEnd[0] }; }
			break;
		default: break;
	}
	return (SyntaxError){ SyntaxErrorNone };
}

static SyntaxError ParseExpression(list(Token) tokens, int32_t start, int32_t end, Expression ** expression)
{
	// end < start occurs when there is no expression where one is expected
	if (end < start) { return (SyntaxError){ SyntaxErrorMissingExpression, end, start }; }
	
	// find the operator index in order of precedence
	SyntaxError error = { SyntaxErrorNone };
	int32_t opIndex = -1;
	int32_t precedence = 6;
	while (precedence > 0)
	{
		switch (precedence)
		{
			case 6: error = FindOperator(tokens, start, end, (const char *[]){ "for", "..." }, 2, true, &opIndex); break;
			case 5: error = FindOperator(tokens, start, end, (const char *[]){ "BL+", "BL-" }, 2, true, &opIndex); break;
			case 4: error = FindOperator(tokens, start, end, (const char *[]){ "*", "/", "%" }, 3, true, &opIndex); break;
			case 3: error = FindOperator(tokens, start, end, (const char *[]){ "^", "UL+", "UL-" }, 3, false, &opIndex); break;
			case 2: error = FindOperator(tokens, start, end, (const char *[]){ "." }, 1, true, &opIndex); break;
			case 1: error = FindFunctionCall(tokens, start, end, &opIndex); break;
			default: break;
		}
		if (error.code != SyntaxErrorNone) { return error; }
		if (opIndex > -1) { break; }
		precedence--;
	}
	
	// parse each operon depending on what operator was found
	*expression = malloc(sizeof(Expression));
	**expression = (Expression){ 0 };
	Operator operator = OperatorType(tokens[opIndex].value, precedence);
	// OperatorNone means a single factor
	if (operator == OperatorNone)
	{
		OperonType operonType = DetermineOperonType(tokens, start, end);
		Operon operon = { 0 };
		if (operonType == OperonTypeExpression)
		{
			// if it occurs that the operon is an expression yet it isn't surrounded by (), then it's not a comprehensible expression
			if (tokens[start].value[0] != '(' || FindCorrespondingBracket(tokens, start, end, start) != end) { return (SyntaxError){ SyntaxErrorNonsenseExpression, start, end }; }
			error = ReadOperon(tokens, start + 1, end - 1, operonType, &operon);
		}
		else { error = ReadOperon(tokens, start, end, operonType, &operon); }
		(*expression)->operator = operator;
		(*expression)->operonTypes[0] = operonType;
		(*expression)->operons[0] = operon;
		(*expression)->operonStart[0] = start;
		(*expression)->operonEnd[0] = end;
	}
	// reads operon of unary operator
	else if (IsUnaryOperator(operator))
	{
		OperonType operonType = DetermineOperonType(tokens, opIndex + 1, end);
		if (operonType == OperonTypeNone) { return (SyntaxError){ SyntaxErrorMissingOperon, start, end }; }
		Operon operon = { 0 };
		error = ReadOperon(tokens, opIndex + 1, end, operonType, &operon);
		(*expression)->operator = operator;
		(*expression)->operonTypes[0] = operonType;
		(*expression)->operons[0] = operon;
		(*expression)->operonStart[0] = opIndex + 1;
		(*expression)->operonEnd[0] = end;
	}
	// reads both operons for binary operator
	else
	{
		if (operator == OperatorFunctionCall) { end--; } // disregard ending ')'
		OperonType leftOperonType = DetermineOperonType(tokens, start, opIndex - 1);
		OperonType rightOperonType = operator == OperatorFunctionCall ? OperonTypeArguments : DetermineOperonType(tokens, opIndex + 1, end); // conditional ensures functions with 1 argument are stil put into a list
		if (leftOperonType == OperonTypeNone) { return (SyntaxError){ SyntaxErrorMissingOperon, opIndex, end }; }
		if (rightOperonType == OperonTypeNone) { return (SyntaxError){ SyntaxErrorMissingOperon, start, opIndex }; }
		Operon leftOperon = { 0 }, rightOperon = { 0 };
		
		error = ReadOperon(tokens, start, opIndex - 1, leftOperonType, &leftOperon);
		if (error.code != SyntaxErrorNone) { return error; }
		error = ReadOperon(tokens, opIndex + 1, end, rightOperonType, &rightOperon);
		
		(*expression)->operator = operator;
		(*expression)->operonTypes[0] = leftOperonType;
		(*expression)->operonTypes[1] = rightOperonType;
		(*expression)->operons[0] = leftOperon;
		(*expression)->operons[1] = rightOperon;
		(*expression)->operonStart[0] = start;
		(*expression)->operonEnd[0] = opIndex - 1;
		(*expression)->operonStart[1] = opIndex + 1;
		(*expression)->operonEnd[1] = end;
	}
	
	// check operator logic before returning
	if (error.code == SyntaxErrorNone) { error = CheckOperatorLogic(*expression, tokens, opIndex); }
	return error;
}

static void FreeExpression(Expression * expression)
{
	// null checks are used here since an expression that terminates early due to an error won't be fully initialized
	if (expression == NULL) { return; }
	for (int32_t i = 0; i < sizeof(expression->operons) / sizeof(expression->operons[0]); i++)
	{
		OperonType type = expression->operonTypes[i];
		if (type == OperonTypeArguments || type == OperonTypeVectorLiteral || type == OperonTypeArrayLiteral)
		{
			if (expression->operons[i].expressions != NULL)
			{
				for (int32_t j = 0; j < ListLength(expression->operons[i].expressions); j++)
				{
					FreeExpression(expression->operons[i].expressions[j]);
				}
				ListFree(expression->operons[i].expressions);
			}
		}
		if (type == OperonTypeExpression) { FreeExpression(expression->operons[i].expression); }
		if (type == OperonTypeIdentifier) { StringFree(expression->operons[i].identifier); }
		if (type == OperonTypeForAssignment)
		{
			StringFree(expression->operons[i].assignment.identifier);
			FreeExpression(expression->operons[i].assignment.expression);
		}
	}
	free(expression);
}

Statement * ParseTokenLine(list(Token) tokens)
{
	Statement * statement = malloc(sizeof(Statement));
	*statement = (Statement){ .error = SyntaxErrorNone, .tokens = tokens, };
	
	// check for any unknown tokens
	for (int32_t i = 0; i < ListLength(tokens); i++)
	{
		if (tokens[i].type == TokenTypeUnknown)
		{
			statement->error = (SyntaxError){ SyntaxErrorUnknownToken, i, i };
			return statement;
		}
	}
	
	// parse statement declaration
	statement->type = DetermineStatementType(tokens);
	int32_t declarationEndIndex = -1;
	switch (statement->type)
	{
		case StatementTypeVariable:
			statement->error = ParseVariableDeclaration(tokens, &statement->declaration, &declarationEndIndex);
			break;
		case StatementTypeFunction:
			statement->error = ParseFunctionDeclaration(tokens, &statement->declaration, &declarationEndIndex);
			break;
		case StatementTypeRender:
			statement->error = ParseRenderDeclaration(tokens, &statement->declaration, &declarationEndIndex);
		default: break; // in the case of StatementTypeUnknown then assume it's just a standalone expression
	}
	if (statement->error.code != SyntaxErrorNone) { return statement; }
	
	// parse the expression
	int32_t expressionStartIndex = declarationEndIndex + 1;
	int32_t expressionEndIndex = ListLength(tokens) - 1;
	statement->error = ParseExpression(tokens, expressionStartIndex, expressionEndIndex, &statement->expression);
	
	return statement;
}

void FreeStatement(Statement * statement)
{
	if (statement->type == StatementTypeVariable) { StringFree(statement->declaration.variable.identifier); }
	if (statement->type == StatementTypeFunction)
	{
		StringFree(statement->declaration.function.identifier);
		if (statement->declaration.function.arguments != NULL)
		{
			for (int32_t i = 0; i < ListLength(statement->declaration.function.arguments); i++)
			{
				StringFree(statement->declaration.function.arguments[i]);
			}
			ListFree(statement->declaration.function.arguments);
		}
	}
	FreeExpression(statement->expression);
	free(statement);
}
