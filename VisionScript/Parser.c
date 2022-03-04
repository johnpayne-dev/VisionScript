#include <stdlib.h>
#include <string.h>
#include "Parser.h"

static StatementType DetermineStatementType(TokenStatement statement)
{
	if (ListLength(statement) < 2) { return StatementTypeUnknown; }
	if (statement[0].type == TokenTypeIdentifier)
	{
		if (statement[1].type == TokenTypeBracket && statement[1].value[0] == '(')
		{
			return StatementTypeFunction;
		}
		if (statement[1].type == TokenTypeSymbol && statement[1].value[0] == '=')
		{
			return StatementTypeVariable;
		}
		return StatementTypeUnknown;
	}
	if (statement[0].type == TokenTypeKeyword)
	{
		if (StringEquals(statement[0].value, "point") || StringEquals(statement[0].value, "parametric") || StringEquals(statement[0].value, "polygon"))
		{
			return StatementTypeRender;
		}
	}
	return StatementTypeUnknown;
}

static SyntaxError ParseVariableDeclaration(TokenStatement tokens, StatementDeclaration * declaration, int32_t * declarationEndIndex)
{
	if (ListLength(tokens) < 2) { return SyntaxErrorInvalidVariableDeclaration; }
	if (tokens[1].type != TokenTypeSymbol || tokens[1].value[0] != '=') { return SyntaxErrorInvalidVariableDeclaration; }
	declaration->variable.identifier = StringCreate(tokens[0].value);
	*declarationEndIndex = 1;
	return SyntaxErrorNone;
}

static SyntaxError ParseFunctionDeclaration(TokenStatement tokens, StatementDeclaration * declaration, int32_t * declarationEndIndex)
{
	int32_t argumentsEndIndex = 0;
	while (argumentsEndIndex < ListLength(tokens))
	{
		if (tokens[argumentsEndIndex].type == TokenTypeBracket && tokens[argumentsEndIndex].value[0] == ')') { break; }
		argumentsEndIndex++;
	}
	if (argumentsEndIndex >= ListLength(tokens) - 1) { return SyntaxErrorInvalidFunctionDeclaration; }
	if (tokens[argumentsEndIndex + 1].type != TokenTypeSymbol || tokens[argumentsEndIndex + 1].value[0] != '=') { return SyntaxErrorInvalidFunctionDeclaration; }
	
	declaration->function.arguments = ListCreate(sizeof(String), 4);
	for (int32_t i = 2; i < argumentsEndIndex; i++)
	{
		if (tokens[i].type != TokenTypeIdentifier) { return SyntaxErrorInvalidFunctionDeclaration; }
		ListPush((void **)&declaration->function.arguments, &(String){ StringCreate(tokens[i].value) });
		i++;
		
		if (i == argumentsEndIndex) { break; }
		if (tokens[i].type != TokenTypeSymbol || tokens[i].value[0] != ',') { return SyntaxErrorInvalidFunctionDeclaration; }
	}
	declaration->function.identifier = StringCreate(tokens[0].value);
	*declarationEndIndex = argumentsEndIndex + 1;
	return SyntaxErrorNone;
}

static SyntaxError ParseRenderDeclaration(TokenStatement tokens, StatementDeclaration * declaration, int32_t * declarationEndIndex)
{
	if (StringEquals(tokens[0].value, "point")) { declaration->render.type = StatementRenderTypePoint; }
	if (StringEquals(tokens[0].value, "parametric")) { declaration->render.type = StatementRenderTypeParametric; }
	if (StringEquals(tokens[0].value, "polygon")) { declaration->render.type = StatementRenderTypePolygon; }
	*declarationEndIndex = 0;
	return SyntaxErrorNone;
}

static int32_t FindCorrespondingBracket(TokenStatement tokens, int32_t start, int32_t end, int32_t bracketIndex)
{
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

static int32_t FindComma(TokenStatement tokens, int32_t start, int32_t end)
{
	int32_t depth = 0;
	for (int32_t i = start; i <= end; i++)
	{
		if (tokens[i].type == TokenTypeBracket)
		{
			if (tokens[i].value[0] == '(' || tokens[i].value[0] == '[' || tokens[i].value[0] == '{') { depth++; }
			if (tokens[i].value[0] == ')' || tokens[i].value[0] == ']' || tokens[i].value[0] == '}') { depth--; }
			if (depth < 0) { return -1; }
		}
		if (depth == 0 && tokens[i].type == TokenTypeSymbol && tokens[i].value[0] == ',') { return i; }
	}
	return -1;
}

static SyntaxError FindOperator(TokenStatement tokens, int32_t start, int32_t end, const char * operators[], int32_t operatorCount, bool unary, bool rightToLeft, int32_t * index)
{
	*index = -1;
	int32_t i = rightToLeft ? end : start;
	while (rightToLeft ? (i >= start) : (i <= end))
	{
		if (tokens[i].type == TokenTypeBracket)
		{
			int32_t bracketIndex = FindCorrespondingBracket(tokens, start, end, i);
			if (bracketIndex < 0) { return SyntaxErrorMissingBracket; }
			i = bracketIndex + (rightToLeft ? -1 : 1);
			continue;
		}
		
		if (tokens[i].type == TokenTypeOperator)
		{
			bool found = false;
			for (int32_t j = 0; j < operatorCount; j++)
			{
				if (StringEquals(tokens[i].value, operators[j]))
				{
					if (unary && i != start && tokens[i - 1].type != TokenTypeOperator) { continue; }
					if (!unary && (i == start || tokens[i - 1].type == TokenTypeOperator)) { continue; }
					found = true;
					break;
				}
			}
			if (found) { *index = i; break; }
		}
		i += rightToLeft ? -1 : 1;
	}
	return SyntaxErrorNone;
}

static SyntaxError FindFunctionCall(TokenStatement tokens, int32_t start, int32_t end, int32_t * index)
{
	*index = -1;
	int32_t i = start;
	while (i <= end)
	{
		if (tokens[i].type == TokenTypeBracket)
		{
			if (tokens[i].value[0] == '(' && i > 0 && tokens[i - 1].type == TokenTypeIdentifier) { *index = i; }
			int32_t bracketIndex = FindCorrespondingBracket(tokens, start, end, i);
			if (bracketIndex < 0) { return SyntaxErrorMissingBracket; }
			i = bracketIndex + 1;
			continue;
		}
		i++;
	}
	return SyntaxErrorNone;
}

static OperonType DetermineOperonType(TokenStatement tokens, int32_t start, int32_t end)
{
	if (tokens[start].value[0] == '(' && FindCorrespondingBracket(tokens, start, end, start) == end && FindComma(tokens, start + 1, end - 1) >= 0) { return OperonTypeVectorLiteral; }
	if (tokens[start].value[0] == '[' && FindCorrespondingBracket(tokens, start, end, start) == end) { return OperonTypeArrayLiteral; }
	if (start == end && tokens[start].type == TokenTypeIdentifier) { return OperonTypeIdentifier; }
	if (start == end && tokens[start].type == TokenTypeNumber) { return OperonTypeConstant; }
	if (end - start > 1 && tokens[start].type == TokenTypeIdentifier && tokens[start + 1].value[0] == '=') { return OperonTypeForAssignment; }
	if (FindComma(tokens, start, end) >= 0) { return OperonTypeArguments; }
	return OperonTypeExpression;
}

static SyntaxError ReadOperon(TokenStatement tokens, int32_t start, int32_t end, OperonType type, Operon * operon)
{
	SyntaxError error = SyntaxErrorNone;
	if (type == OperonTypeExpression) { error = ParseExpression(tokens, start, end, &operon->expression); }
	if (type == OperonTypeIdentifier) { operon->identifier = StringCreate(tokens[start].value); }
	if (type == OperonTypeConstant) { operon->constant = atof(tokens[start].value); }
	if (type == OperonTypeForAssignment)
	{
		operon->forAssignment.identifier = StringCreate(tokens[start].value);
		error = ParseExpression(tokens, start + 2, end, &operon->forAssignment.expression);
	}
	if (type == OperonTypeArguments || type == OperonTypeArrayLiteral || type == OperonTypeVectorLiteral)
	{
		if (type == OperonTypeArrayLiteral || type == OperonTypeVectorLiteral) { start += 1; end -= 1; }
		operon->expressions = ListCreate(sizeof(Expression *), 4);
		int32_t prevStart = start;
		int32_t commaIndex = FindComma(tokens, start, end);
		int32_t elementIndex = 0;
		while (commaIndex > -1)
		{
			if (type == OperonTypeVectorLiteral && elementIndex >= 3) { error = SyntaxErrorTooManyVectorElements; break; }
			ListPush((void **)&operon->expressions, &(Expression *){ NULL });
			error = ParseExpression(tokens, prevStart, commaIndex - 1, &operon->expressions[elementIndex]);
			if (error != SyntaxErrorNone) { return error; }
			prevStart = commaIndex + 1;
			commaIndex = FindComma(tokens, commaIndex + 1, end);
			elementIndex++;
		}
		ListPush((void **)&operon->expressions, &(Expression *){ NULL });
		error = ParseExpression(tokens, prevStart, end, &operon->expressions[elementIndex]);
	}
	return error;
}

static Operator OperatorType(String string, int32_t precedence)
{
	if (precedence == 0) { return OperatorNone; }
	if (StringEquals(string, "for")) { return OperatorFor; }
	if (StringEquals(string, "...")) { return OperatorEllipsis; }
	if (StringEquals(string, "+"))
	{
		if (precedence == 6) { return OperatorAdd; }
		else { return OperatorPositive; }
	}
	if (StringEquals(string, "-"))
	{
		if (precedence == 6) { return OperatorSubtract; }
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

static bool InsideArray(TokenStatement tokens, int32_t index)
{
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

static SyntaxError CheckOperatorLogic(Expression * expression, TokenStatement tokens, int32_t opIndex)
{
	if (expression->operonTypes[0] == OperonTypeArguments) { return SyntaxErrorInvalidCommaPlacement; }
	if (expression->operonTypes[1] == OperonTypeArguments && expression->operator != OperatorFunctionCall) { return SyntaxErrorInvalidCommaPlacement; }
	if (expression->operonTypes[0] == OperonTypeForAssignment) { return SyntaxErrorInvalidForAssignmentPlacement; }
	if (expression->operonTypes[1] == OperonTypeForAssignment && expression->operator != OperatorFor) { return SyntaxErrorInvalidForAssignmentPlacement; }
	switch (expression->operator)
	{
		case OperatorFor:
			if (!InsideArray(tokens, opIndex)) { return SyntaxErrorInvalidForPlacement; }
			if (expression->operonTypes[1] != OperonTypeForAssignment) { return SyntaxErrorInvalidForAssignment; }
			break;
		case OperatorEllipsis:
			if (!InsideArray(tokens, opIndex)) { return SyntaxErrorInvalidEllipsisPlacement; }
			if (expression->operonTypes[0] == OperonTypeArrayLiteral || expression->operonTypes[1] == OperonTypeArrayLiteral) { return SyntaxErrorInvalidEllipsisOperon; }
			if (expression->operonTypes[0] == OperonTypeVectorLiteral || expression->operonTypes[1] == OperonTypeVectorLiteral) { return SyntaxErrorInvalidEllipsisOperon; }
			break;
		case OperatorIndex:
			if (expression->operonTypes[0] == OperonTypeConstant) { return SyntaxErrorIndexingConstant; }
			if (expression->operonTypes[1] == OperonTypeConstant) { return SyntaxErrorIndexingWithConstant; }
			if (expression->operonTypes[1] == OperonTypeVectorLiteral) { return SyntaxErrorIndexingWithVector; }
			break;
		case OperatorFunctionCall:
			if (expression->operonTypes[0] != OperonTypeIdentifier) { return SyntaxErrorInvalidFunctionCall; }
			break;
		default: break;
	}
	return SyntaxErrorNone;
}

SyntaxError ParseExpression(TokenStatement tokens, int32_t start, int32_t end, Expression ** expression)
{
	if (end < start) { return SyntaxErrorMissingExpression; }
	
	SyntaxError error = SyntaxErrorNone;
	int32_t opIndex = -1;
	int32_t precedence = 7;
	while (precedence > 0)
	{
		switch (precedence)
		{
			case 7: error = FindOperator(tokens, start, end, (const char *[]){ "for", "..." }, 2, false, true, &opIndex); break;    // binary, right to left
			case 6: error = FindOperator(tokens, start, end, (const char *[]){ "+", "-" }, 2, false, false, &opIndex); break;       // binary, left to right
			case 5: error = FindOperator(tokens, start, end, (const char *[]){ "*", "/", "%" }, 3, false, false, &opIndex); break;  // binary, left to right
			case 4: error = FindOperator(tokens, start, end, (const char *[]){ "+", "-" }, 2, true, false, &opIndex); break;        // unary,  left to right
			case 3: error = FindOperator(tokens, start, end, (const char *[]){ "^" }, 1, false, false, &opIndex); break;            // binary, left to right
			case 2: error = FindOperator(tokens, start, end, (const char *[]){ "." }, 1, false, true, &opIndex); break;             // binary, right to left
			case 1: error = FindFunctionCall(tokens, start, end, &opIndex); break;
			default: break;
		}
		if (error != SyntaxErrorNone) { return error; }
		if (opIndex > -1) { break; }
		precedence--;
	}
	
	*expression = malloc(sizeof(Expression));
	Operator operator = OperatorType(tokens[opIndex].value, precedence);
	if (operator == OperatorNone)
	{
		OperonType operonType = DetermineOperonType(tokens, start, end);
		Operon operon = { 0 };
		if (operonType == OperonTypeExpression)
		{
			if (tokens[start].value[0] != '(' || FindCorrespondingBracket(tokens, start, end, start) != end) { return SyntaxErrorMissingOperator; }
			error = ReadOperon(tokens, start + 1, end - 1, operonType, &operon);
		}
		else { error = ReadOperon(tokens, start, end, operonType, &operon); }
		(*expression)->operator = operator;
		(*expression)->operonTypes[0] = operonType;
		(*expression)->operons[0] = operon;
	}
	else if (IsUnaryOperator(operator))
	{
		OperonType operonType = DetermineOperonType(tokens, opIndex + 1, end);
		Operon operon = { 0 };
		error = ReadOperon(tokens, opIndex + 1, end, operonType, &operon);
		(*expression)->operator = operator;
		(*expression)->operonTypes[0] = operonType;
		(*expression)->operons[0] = operon;
	}
	else
	{
		if (operator == OperatorFunctionCall) { end--; }
		OperonType leftOperonType = DetermineOperonType(tokens, start, opIndex - 1);
		OperonType rightOperonType = DetermineOperonType(tokens, opIndex + 1, end);
		Operon leftOperon = { 0 }, rightOperon = { 0 };
		
		error = ReadOperon(tokens, start, opIndex - 1, leftOperonType, &leftOperon);
		if (error != SyntaxErrorNone) { return error; }
		error = ReadOperon(tokens, opIndex + 1, end, rightOperonType, &rightOperon);
		
		(*expression)->operator = operator;
		(*expression)->operonTypes[0] = leftOperonType;
		(*expression)->operonTypes[1] = rightOperonType;
		(*expression)->operons[0] = leftOperon;
		(*expression)->operons[1] = rightOperon;
	}
	
	if (error != SyntaxErrorNone) { error = CheckOperatorLogic(*expression, tokens, opIndex); }
	return error;
}

Statement * ParseTokenStatement(TokenStatement tokens)
{
	Statement * statement = malloc(sizeof(Statement));
	*statement = (Statement){ .error = SyntaxErrorNone };
	
	for (int32_t i = 0; i < ListLength(tokens); i++)
	{
		if (tokens[i].type == TokenTypeUnknown)
		{
			statement->error = SyntaxErrorUnknownToken;
			return statement;
		}
	}
	
	statement->type = DetermineStatementType(tokens);
	if (statement->type == StatementTypeUnknown)
	{
		statement->error = SyntaxErrorUnknownStatementType;
		return statement;
	}
	
	int32_t declarationEndIndex = 0;
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
		default: break;
	}
	if (statement->error != SyntaxErrorNone) { return statement; }
	
	int32_t expressionStartIndex = declarationEndIndex + 1;
	int32_t expressionEndIndex = ListLength(tokens) - 1;
	statement->error = ParseExpression(tokens, expressionStartIndex, expressionEndIndex, &statement->expression);
	
	return statement;
}

void DestroyStatement(Statement * statement)
{
	free(statement);
}
