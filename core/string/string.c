/*
 * string.c - This file implementes different string
 * functions found in string.h
 *
 * Author: Fedi Nabli
 * Date: 26 Mar 2025
 * Last Modified: 26 Mar 2025
 */

#include <synapse/string/string.h>

#include <synapse/bool.h>
#include <synapse/types.h>

/**
 * @brief Converts an upper case character to lower case
 * 
 * @param c character in lower case
 * @return char character in upper case
 * 
 * @author Fedi Nabli
 * @date 26 Mar 2025
 */
char tolower(char c)
{
  if (c >= 65 && c <= 90)
  {
    c += 32;
  }

  return c;
}

/**
 * @brief Converts a lower case character to upper case
 * 
 * @param c Character in upper case
 * @return char Character in lower case
 * 
 * @author Fedi Nabli
 * @date 26 Mar 2025
 */
char toupper(char c)
{
  if (c >= 97 && c <= 122)
  {
    c -= 32;
  }

  return c;
}

/**
 * @brief Returns the length of a string
 * 
 * @param str The string
 * @return size_t length of the string
 * 
 * @author Fedi Nabli
 * @date 26 Mar 2025
 */
size_t strlen(const char* str)
{
  int i = 0;
  while (*str != 0)
  {
    i++;
    str += 1;
  }

  return i;
}

/**
 * @brief Returns the length of a string or the max if it exceeds the maximum
 * 
 * @param str The string 
 * @param max Maximum length
 * @return size_t Size of the string or maximum provided if it exceeds it
 * 
 * @author Fedi Nabli
 * @date 26 Mar 2025
 */
size_t strnlen(const char* str, int max)
{
  int i = 0;
  for (i = 0; i < max; i++)
  {
    if (str[i] == 0)
      break;
  }

  return i;
}

/**
 * @brief Counts length of a string until reaching maximum or finds terminator
 * 
 * @param str The string
 * @param max Maximum length
 * @param terminator Termination length character
 * @return size_t Length of the string
 * 
 * @author Fedi Nabli
 * @date 26 Mar 2025
 */
size_t strnlen_terminator(const char* str, int max, char terminator)
{
  int i = 0;
  for (i = 0; i < max; i++)
  {
    if (str[i] == '\0' || str[i] == terminator)
      break;
  }

  return i;
}

/**
 * @brief Compares two strings ignoring case up to n characters
 * 
 * @param s1 First string
 * @param s2 Second string
 * @param n Maximum number of characters to compare
 * @return int Difference between the first differing characters, 0 if equal
 * 
 * @author Fedi Nabli
 * @date 26 Mar 2025
 */
int istrncmp(const char* s1, const char* s2, int n)
{
  unsigned char u1, u2;

  while (n-- > 0)
  {
    u1 = (unsigned char)*s1++;
    u2 = (unsigned char)*s2++;

    if (u1 != u2 && tolower(u1) != tolower(u2))
      return u1 - u2;

    if (u1 == '\0')
      return 0;
  }

  return 0;
}

/**
 * @brief Compares two strings up to n characters
 * 
 * @param str1 First string
 * @param str2 Second string
 * @param n Maximum number of characters to compare
 * @return int Difference between the first differing characters, 0 if equal
 * 
 * @author Fedi Nabli
 * @date 26 Mar 2025
 */
int strncmp(const char* str1, const char* str2, int n)
{
  unsigned char u1, u2;

  while (n-- > 0)
  {
    u1 = (unsigned char)*str1++;
    u2 = (unsigned char)*str2++;

    if (u1 != u2)
      return u1 - u2;

    if (u1 == '\0')
      return 0;
  }

  return 0;
}

/**
 * @brief Function to copy string from one variable to another
 * 
 * @param dest The destination string
 * @param src The source string
 * @return char* Pointer to the destination string
 * 
 * @author Fedi Nabli
 * @date 26 Mar 2025
 */
char* strcpy(char* dest, const char* src)
{
  char* res = dest;
  while (*src != 0)
  {
    *dest = *src;
    src += 1;
    dest += 1;
  }

  *dest = 0x00;

  return res;
}

/**
 * @brief Copies up to count characters from the source string to the destination string
 * 
 * @param dest The destination string
 * @param src The source string
 * @param count The maximum number of characters to copy
 * @return char* Pointer to the destination string
 * 
 * @author Fedi Nabli
 * @date 26 Mar 2025
 */
char* strncpy(char* dest, const char* src, int count)
{
  int i = 0;
  for (i = 0; i < count; i++)
  {
    if (src[i] == 0x00)
      break;

    dest[i] = src[i];
  }

  for (; i < count; i++)
  {
    dest[i] = 0x00;
  }

  return dest;
}

/**
 * @brief Checks if a character is a digit
 * 
 * @param c Character to check
 * @return true if the character is a digit
 * @return false if the character is not a digit
 * 
 * @author Fedi Nabli
 * @date 26 Mar 2025
 */
bool isdigit(char c)
{
  return c >= 48 && c <= 57;
}

/**
 * @brief Converts a character digit to its numeric value
 * 
 * @param c Character to convert
 * @return int Numeric value of the character
 * 
 * @author Fedi Nabli
 * @date 26 Mar 2025
 */
int tonumericdigit(char c)
{
  return c - 48;
}
