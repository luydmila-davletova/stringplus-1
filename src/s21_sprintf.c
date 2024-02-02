#include <stdarg.h>

#include "s21_string.h"

int s21_sprintf(char *str, const char *format, ...) {
  char specifiers[] = "dfsgGeExXcuiopn%";
  char *src = str;

  va_list arguments;
  va_start(arguments, format);

  while (*format != '\0') {
    if (*format == '%') {
      format++;
      Spec specs = {0};
      specs.number_system = 10;
      format = set_specs(&specs, format, arguments);
      while (!s21_strchr(specifiers, *format)) format++;
      str = read_spec(str, src, format, specs, arguments);
    } else {
      *str = *format;
      str++;
    }
    format++;
  }
  *str = '\0';
  va_end(arguments);
  return (str - src);
}

const char *get_specs(const char *format, Spec *specs) {
  while (format) {
    if (*format == '+') {
      specs->plus = 1;
    } else if (*format == '-') {
      specs->minus = 1;
    } else if (*format == '#') {
      specs->hash = 1;
    } else if (*format == ' ') {
      specs->space = 1;
    } else if (*format == '0') {
      specs->full_zero = 1;
    } else
      break;
    format++;
  }

  specs->space = (specs->space && !specs->plus);
  specs->full_zero = (specs->full_zero && !specs->minus);
  return format;
}

const char *get_width(const char *format, int *width, va_list arguments) {
  *width = 0;
  if (*format == '*') {
    *width = va_arg(arguments, int);
    format++;
  }

  while (format) {
    if ('0' <= *format && *format <= '9') {
      *width *= 10;
      *width += *format - '0';
    } else
      break;
    format++;
  }
  // точка находится вне
  return format;
}

//-+014.7 проверка set_specs
const char *set_specs(Spec *specs, const char *format, va_list arguments) {
  format = get_specs(format, specs);
  format = get_width(format, &specs->width, arguments);
  if (*format == '.') {
    specs->dot = 1;
    specs->full_zero = 0;
    format += 1;
    format = get_width(format, &specs->accuracy, arguments);
  } else if (*format != 'd')
    specs->accuracy = 6;
  if (*format == 'L') specs->length = 'L';
  if (*format == 'l')
    specs->length = 'l';
  else if (*format == '1')
    specs->length = '1';
  else if (*format == 'h')
    specs->length = 'h';
  if (specs->length != 0) format += 1;

  if (specs->width < 0) {
    specs->width = -specs->width;
    specs->minus = 1;
  }
  return format;
}

Spec place_number_system(Spec specs, char format) {
  if (format == 'o')
    specs.number_system = 8;
  else if (format == 'x' || format == 'X')
    specs.number_system = 16;
  if (format == 'X') specs.upper_case = 1;
  return specs;
}

Spec num_sys_double(Spec specs, char format) {
  if (format == 'g' || format == 'G') specs.g = 1;
  // setting registers for upper case and g e
  else if (format == 'e' || format == 'E')
    specs.e = 1;
  if (format == 'G' || format == 'E') specs.upper_case = 1;
  return specs;
}

// СПЕЦИФИКАТОРЫ!
char *read_spec(char *str, char *src, const char *format, Spec specs,
                va_list arguments) {
  if (*format == 'c') {
    int symbol =
        va_arg(arguments, int);  // не работает с char или плохо работает
    str = print_char(str, specs,
                     symbol);  // не можем просто написать *str =  symbol ;
  } else if (*format == 'd' || *format == 'i') {
    str = print_decimal(str, specs, arguments);
  } else if (*format == 'u' || *format == 'o' || *format == 'x' ||
             *format == 'X') {
    specs = place_number_system(specs, *format);
    // filling the number-system
    str = spec_print_u(str, specs, *(format - 1), arguments);
    // transform number in str-format
  } else if (*format == 's') {
    str = print_s(str, specs, arguments);
  } else if (*format == 'p') {
    str = print_p(str, &specs, arguments);
  } else if (*format == 'n') {
    int *n = va_arg(arguments, int *);
    *n = (int)(str - src);
  } else if (*format == 'f') {
    specs = num_sys_double(specs, *format);
    str = print_double(str, specs, *(format - 1), arguments);
  } else if (*format == 'e' || *format == 'E' || *format == 'g' ||
             *format == 'G') {
    specs = num_sys_double(specs, *format);
    str = print_e_g_double(str, specs, *(format - 1), arguments);
  } else if (*format == '%') {
    str = print_char(str, specs, '%');
  } else {
    str = NULL;
  }
  if (!str) *str = '\0';
  return str;
}

char *print_char(char *str, Spec specs, int symbol) {
  char *ptr = S21_NULL;
  int i = 0;  // подсчет символов

  // проверка на ширину и минус
  while (specs.width - 1 > 0 && !specs.minus) {
    *str = ' ';
    str++;
    i++;
    specs.width--;
  }
  if (symbol <= 127) {
    *str = symbol;
    str++;
    i++;
    while (specs.width - 1 > 0 && specs.minus) {
      *str = ' ';
      str++;
      i++;
      specs.width--;
    }
    ptr = str;
  }
  return ptr;
}

char *print_s(char *str, Spec specs, va_list arguments) {
  char *ptr = str;
  char *string = va_arg(arguments, char *);

  // Если удалось получить строку
  if (string) {
    int tmp = specs.width;
    // Сохраняем исходное значение ширины
    if ((s21_size_t)specs.width < s21_strlen(string)) {
      specs.width = s21_strlen(string);
    }
    int space = specs.width - s21_strlen(string);
    if (specs.accuracy == 0 && !specs.dot) {
      specs.accuracy = tmp;  // Используем исходное значение ширины
    }
    if (specs.accuracy == 0 && specs.dot) {
      specs.accuracy = 0;  // Используем исходное значение ширины
      space = specs.width;
    }
    if (specs.accuracy != 0 && specs.accuracy < tmp &&
        specs.accuracy < (int)s21_strlen(string)) {
      //  Рассчитываем сколько пробелов нам нужно добавить
      space = tmp - specs.accuracy;
    } else if (specs.accuracy > (int)s21_strlen(string)) {
      space = tmp - (int)s21_strlen(string);
      if (space < 0) space = 0;
    }
    // Заполняем пробелами слева
    while (space && !specs.minus) {
      *str = ' ';
      str++;
      space--;
    }
    // Посимвольно копируем из переменной string в str
    int i = 0;
    while (*string != '\0' && specs.accuracy) {
      *str = *string;
      str++;
      string++;
      i++;
      specs.accuracy--;
    }
    // Заполняем пробелами справа, если был указан флаг "-"
    while (space != 0 && specs.minus) {
      *str = ' ';
      str++;
      space--;
    }
    // Если не удалось получить строку из параметра, пишем в нашу строку null
  } else {
    str = s21_memcpy(str, "(null)", 6);
    str += 6;
  }
  // Присваиваем ptr str и возвращаем его, чтобы передвинуть указатель строки
  if (ptr) {
    ptr = str;
  }
  return ptr;
}

char *print_p(char *str, Spec *specs, va_list arguments) {
  // получаем адрес из аргументов
  unsigned long int ptr =
      (unsigned long int)va_arg(arguments, unsigned long int);
  // устанавливаем параметры для 16- системы что бы строка имела вид 8х
  specs->number_system = 16;
  specs->hash = 1;
  specs->upper_case = 0;
  s21_size_t size_to_num = size_unsigned_decimal(
      specs, ptr);  // исправлено с size_unsigned_decimal указетель кричит
  char *buffer = malloc(sizeof(char) * size_to_num);
  // get_buf_size_unsigned_decimal
  if (buffer) {
    int i =
        unsigned_decimal_string(buffer, *specs, ptr, size_to_num);  // исправить
    // reversing buffer string to majot str
    for (int j = i - 1; j >= 0; j--) {
      *str = buffer[j];
      str++;
    }
    while (i < specs->width) {
      *str = ' ';
      str++;
      i++;  // i am not sure about this string (188), but i think without it we
            // will have eternal cycle
    }
  }
  if (buffer) free(buffer);
  return str;
}

char *spec_print_u(char *str, Spec specs, char format, va_list arguments) {
  unsigned long int num = 0;
  // по типу переменной разбираем текущий спецификатор (long, short, int) with
  //"unsigned" word
  if (format == 'l')
    num = (unsigned long int)va_arg(arguments, unsigned long int);
  else
    num = (unsigned int)va_arg(arguments, unsigned int);
  s21_size_t size_num = size_unsigned_decimal(&specs, num);
  char *buf_str = malloc(sizeof(char) * size_num);
  if (buf_str) {
    int i = unsigned_decimal_string(buf_str, specs, num, size_num);
    // reversing buffer string to majot str
    for (int j = i - 1; j >= 0; j--) {
      *str = buf_str[j];
      str++;
    }
    while (i < specs.width) {
      *str = ' ';
      str++;
      i++;  // i am not sure about this string (188), but i think without it we
            // will have eternal cycle
    }
  }
  if (buf_str) free(buf_str);
  return str;
}

char get_num_char(int num, int upper_case) {
  char flag = '0';
  switch (num) {
    // if if has numerical sys more than 10...
    case 10:
      flag = (char)('a' - upper_case * 32);
      break;
    case 11:
      flag = (char)('b' - upper_case * 32);
      break;
    case 12:
      flag = (char)('c' - upper_case * 32);
      break;
    case 13:
      flag = (char)('d' - upper_case * 32);
      break;
    case 14:
      flag = (char)('e' - upper_case * 32);
      break;
    case 15:
      flag = (char)('f' - upper_case * 32);
      break;
  }
  // and less than 11
  if (0 <= num && 9 >= num) flag = (char)(num + 48);
  return flag;
}

char *print_decimal(char *str, Spec specs, va_list arguments) {
  long int num = 0;
  if (specs.length == 'l') {  // по типу переменной разбираем текущий
    // спецификатор (long, short, int)
    num = (long int)va_arg(arguments, long int);
  } else {
    num = (int)va_arg(arguments, int);
  }
  s21_size_t size_decimal = get_size_decimal(&specs, num);
  // вызов функции для подсчета буфера для числа (последовательное деление на
  // 10)
  char *str_num = malloc(sizeof(char) * size_decimal);
  // allocating memory for string with size of result we had of integer if
  // size_decimal
  if (str_num)  // placing symbols in reverse order in str_num
  {
    int i = decimal_string(specs, num, str_num, size_decimal);
    // reversing buffer string to majot str
    for (int j = i - 1; j >= 0; j--) {
      *str = str_num[j];
      str++;
    }
    while (i < specs.width) {
      *str = ' ';
      str++;
      i++;  // i am not sure about this string (188), but i think without it we
            // will have eternal cycle
    }
  }
  if (str_num) free(str_num);
  return str;
}

int decimal_string(Spec specs, long int num, char *str_num,
                   s21_size_t size_decimal) {
  int flag = 0, i = 0;
  if (num < 0) {
    num = -num;
    flag = 1;
  }
  long int copy_num = num;
  i = decimal_string_zeros(specs, num, str_num, size_decimal, i, copy_num,
                           flag);
  return i;
}

int decimal_string_zeros(Spec specs, long int num, char *str_num,
                         s21_size_t size_decimal, int i, long int copy_num,
                         int flag) {
  // printing number in a mass if it is 0
  s21_size_t size_decimal_copy = size_decimal;
  if (((copy_num == 0) && (specs.accuracy || specs.width || specs.space)) ||
      (copy_num == 0 && !specs.accuracy && !specs.width && !specs.space &&
       !specs.dot)) {
    char symbol = copy_num % specs.number_system + '0';
    // transforming num to sym by dividing to numerical system and adding 0
    // symbol
    str_num[i] = symbol;
    i++;
    size_decimal--;
    copy_num /= 10;
  }
  while (copy_num && str_num && size_decimal) {
    char symbol =
        get_num_char(copy_num % specs.number_system, specs.upper_case);
    str_num[i] = symbol;
    i++;
    size_decimal--;
    copy_num /= 10;
  }
  if (flag) num = -num;
  // at the start of func we had a flag to negative number
  // flag '0' treatment review
  if (specs.accuracy >
      i) {  // accuracy is bigger than number of already printed numbers in str
    specs.accuracy -= i;
    specs.full_zero = 1;
    flag = 0;
  } else
    flag = 1;
  // if we don't have place for zeros turning off flag '0'
  if (size_decimal == 1 && specs.full_zero == 1 && specs.flag_size == 1)
    specs.full_zero = 0;
  // flag '0' treatment
  while (specs.full_zero && str_num && (size_decimal - specs.flag_size > 0) &&
         (specs.accuracy != 0 || flag)) {
    if ((size_decimal == 1 && specs.flag_size == 1) ||
        (!specs.accuracy && flag == 0))
      break;
    str_num[i] = '0';
    i++;
    size_decimal--;
    specs.accuracy--;
  }
  if (specs.space && num >= 0 && size_decimal) {
    str_num[i] = ' ';
    i++;
    size_decimal--;
  } else if (num < 0 && size_decimal) {
    str_num[i] = '-';
    i++;
    size_decimal--;
  } else if (num > 0 && specs.plus && size_decimal) {
    str_num[i] = '+';
    i++;
    size_decimal--;
  }
  // if we still have empty space in our string
  if (size_decimal > 0 && specs.minus == 0) {
    while ((size_decimal_copy - i > 0)) {
      str_num[i] = ' ';
      i++;
      size_decimal--;
    }
  }
  return i;
}

s21_size_t get_size_decimal(Spec *specs, long int num) {
  s21_size_t result = 0;    // returning result
  long int copy_num = num;  // do not want to change num and break it
  if (copy_num < 0)
    copy_num = -copy_num;  // reroll from - num if it is less than 0
  while (copy_num > 0) {
    copy_num /= 10;
    result++;  // iterational changing of copy_num to count number of symbols in
               // it
  }
  if (copy_num == 0 && result == 0 &&
      (specs->accuracy || specs->width || specs->space))
    result++;
  // if we have one of these specs, we need to add one place to symbol even
  if (result < (s21_size_t)specs->width) result = specs->width;
  // if width is bigger we need to get a bigger result to make it equal to width
  if (result < (s21_size_t)specs->accuracy) result = specs->accuracy;
  if ((specs->space || specs->plus || num < 0) &&
      (int)result != specs->accuracy && (int)result != specs->width) {
    specs->flag_size = 1;
    result++;
  }
  if ((specs->width < specs->accuracy) &&
      ((specs->space && !specs->plus && num >= 0) || specs->plus) &&
      (specs->dot == 1))
    result++;
  // analogical to accuracy
  // if we have spec to have additional one place to string or num have '-' in
  // start of it we need to have additional place
  if (result == 0 && copy_num == 0 && !specs->accuracy && !specs->width &&
      !specs->space && !specs->dot)
    result++;
  if (result == 0) result = 1;
  // if we still have result == 0 and other shit result should be 1 (for ex. we
  // had a number "0")
  return result;
}

char *print_double(char *str, Spec specs, char format, va_list arguments) {
  long double number = 0;
  if (format == 'L') number = va_arg(arguments, long double);
  // if last symbol was L (like Lf) -> then set it long double
  // otherwise - double (clear f)
  else
    number = va_arg(arguments, double);
  // size calculating
  s21_size_t size_double = get_size_double(&specs, number);
  char *str_num = malloc(sizeof(char) * size_double);
  // allocating memory for string with size of result we had of integer if
  // size_double
  if (!isnan(number) && !isinf(number)) {
    if (str_num) {
      int i = double_string(str_num, specs, number, size_double);
      int a = i;
      // reversing buffer string to majot str
      while (!specs.minus && (i < specs.width)) {
        *str = ' ';
        str++;
        i++;  // i am not sure about this string, but i think without it we
              // will have eternal cycle
      }
      for (int j = a - 1; j >= 0; j--) {
        *str = str_num[j];
        str++;
      }
      while (i < specs.width) {
        *str = ' ';
        str++;
        i++;  // i am not sure about this string, but i think without it we
              // will have eternal cycle
      }
    }
  } else {
    str = nan_or_inf(str, number, specs, size_double);
  }
  if (str_num) free(str_num);
  return str;
}

char *print_e_g_double(char *str, Spec specs, char format, va_list arguments) {
  long double number = 0, e = 0;
  if (format == 'L') number = va_arg(arguments, long double);
  // if last symbol was L (like Lf) -> then set it long double
  // otherwise - double (clear f)
  else
    number = va_arg(arguments, double);

  s21_size_t size = 0;
  if (specs.g) {
    specs = cutter(specs, number);
  }
  if (specs.e) {
    e = normalize(&number, &specs);
    if (e < 100)
      size += 2;
    else
      size += 3;
  }
  size += get_size_e_g(&specs, number);
  if ((int)size < specs.accuracy) size = specs.accuracy;
  if ((int)size < specs.width) size = specs.width;
  char *str_double = malloc(sizeof(char) * size);
  if (str_double) {
    int i = double_string(str_double, specs, number, size);
    // reversing buffer string to majot str
    for (int j = i - 1; j >= 0; j--) {
      *str = str_double[j];
      str++;
    }
    while (i < specs.width) {
      *str = ' ';
      str++;
      i++;  // i am not sure about this string (188), but i think without it we
            // will have eternal cycle
    }
  }
  if (str_double) free(str_double);
  return str;
}

s21_size_t get_size_e_g(Spec *specs, long double number) {
  s21_size_t result = 0;  // Инициализация размера строки
  long double copy_number = number;  // Создание копии числа

  // Нормализация числа и обновление specs->e
  int normalizationResult = normalize(&copy_number, specs);

  // Определение размера строки на основе нормализации и других параметров
  result += normalizationResult;  // Размер от нормализации
  result += specs->width;  // Дополнительные символы из specs->width
  result += (specs->space || specs->plus ||
             number < 0);  // Учет дополнительных символов

  return result;
}

long double normalize(long double *number, Spec *specs) {
  int i = 0;
  if (fabsl(*number) > 1) {
    while (fabsl(*number) > 10) {
      *number /= 10;
      i++;
      specs->e = 2;
    }
  } else {
    while (fabsl(*number) < 0.999999) {
      if (*number == 0) {
        specs->e = 2;
        break;
      }
      *number *= 10;
      i++;
      specs->e = 1;
    }
  }
  return i;
}

Spec cutter(Spec specs, long double num) {
  Spec copy_specs = specs;
  long double copy_num = num;
  int e = normalize(&copy_num, &copy_specs);
  if (copy_specs.accuracy == 0 && !copy_specs.dot) copy_specs.accuracy = 6;
  if ((e <= 4 && copy_specs.e == 1) || (copy_specs.e == 2 && e < 6))
    copy_specs.e = 0;
  return copy_specs;
}

int double_string(char *str_num, Spec specs, long double number,
                  s21_size_t size_double) {
  int i = 1;
  if (i) {
    i--;
    int flag = 0;
    if (number < 0) {
      flag = 1;
      number = -number;
    }
    long double integer = 0;
    long double frac = modfl(number, &integer);
    int accuracy = specs.accuracy;
    long double copy_frac = frac;
    if (frac + 0.000000000000001 >= 1) {
      frac = 0;
      integer += 1;
    }
    for (int i = 0; i < 15; i++) {
      copy_frac *= 10;
      if ((long)copy_frac != 0) break;
    }
    if ((long)copy_frac == 0 && specs.g) accuracy = 0;
    if (!accuracy && frac * 10 > 4) integer += 1;
    while (accuracy > 0) {
      frac *= 10;
      accuracy--;
    }
    accuracy = specs.accuracy;
    if (fmodl(fabsl(frac) * 10, 10.0) > 4) frac = roundl(frac);
    if (specs.g) {
      while ((long)frac % 10 == 0 && specs.accuracy > 0 && (long)frac != 0) {
        frac /= 10;
        accuracy++;
      }
    }
    int fracc = (int)frac;
    int integerr = (int)integer;
    while (fracc && str_num) {
      char symbol = get_num_char(fracc % specs.number_system, specs.upper_case);
      str_num[i] = symbol;
      i++;
      fracc /= 10;
      accuracy--;
    }
    while (accuracy > 0) {
      str_num[i] = '0';
      i++;
      accuracy--;
    }
    if (specs.accuracy != 0 || specs.hash) {
      str_num[i] = '.';
      i++;
    }
    if (integerr == 0) {
      char symbol = integerr % specs.number_system + '0';
      // transforming num to sym by dividing to numerical system and adding 0
      // symbol
      str_num[i] = symbol;
      i++;
      integerr /= 10;
    }
    // printing number in a mass if it is not 0
    while (integerr && str_num) {
      char symbol =
          get_num_char(integerr % specs.number_system, specs.upper_case);
      str_num[i] = symbol;
      i++;
      integerr /= 10;
    }
    if (flag) number = -number;
    long double copy_num = number;

    if (specs.space && copy_num >= 0 && size_double) {
      str_num[i] = ' ';
      i++;
    } else if (copy_num < 0 && size_double) {
      str_num[i] = '-';
      i++;
    } else if (copy_num > 0 && specs.plus && size_double) {
      str_num[i] = '+';
      i++;
    }
  }
  return i;
}

char *nan_or_inf(char *str, long double number, Spec specs, s21_size_t size) {
  int i = 0;
  if (isnan(number)) {
    while (!specs.minus && (i < specs.width - 3)) {
      *str = ' ';
      str++;
      i++;  // i am not sure about this string, but i think without it we
            // will have eternal cycle
    }
    // Если число - NaN
    if (size < 3) size = 3;
    if (specs.upper_case) {
      *str = 'N';
      str++;
      *str = 'A';
      str++;
      *str = 'N';
      str++;
    } else {
      *str = 'n';
      str++;
      *str = 'a';
      str++;
      *str = 'n';
      str++;
    }
    i += 3;
    while (i < specs.width) {
      *str = ' ';
      str++;
      i++;  // i am not sure about this string, but i think without it we
            // will have eternal cycle
    }
  } else if (isinf(number)) {
    // Если число - бесконечность
    if (specs.upper_case) {
      if (number > 0) {
        while (!specs.minus && (i < specs.width - 3)) {
          *str = ' ';
          str++;
          i++;  // i am not sure about this string, but i think without it we
                // will have eternal cycle
        }
        if (size < 3) size = 3;
        *str = 'I';
        str++;
        *str = 'N';
        str++;
        *str = 'F';
        str++;
        i += 3;
      } else {
        while (!specs.minus && (i < specs.width - 4)) {
          *str = ' ';
          str++;
          i++;  // i am not sure about this string, but i think without it we
                // will have eternal cycle
        }
        if (size < 4) size = 4;
        *str = '-';
        str++;
        *str = 'I';
        str++;
        *str = 'N';
        str++;
        *str = 'F';
        str++;
        i += 4;
      }
    } else {
      if (number > 0) {
        while (!specs.minus && (i < specs.width - 3)) {
          *str = ' ';
          str++;
          i++;  // i am not sure about this string, but i think without it we
                // will have eternal cycle
        }
        if (size < 3) size = 3;
        *str = 'i';
        str++;
        *str = 'n';
        str++;
        *str = 'f';
        str++;
        i += 3;
      } else {
        while (!specs.minus && (i < specs.width - 4)) {
          *str = ' ';
          str++;
          i++;  // i am not sure about this string, but i think without it we
                // will have eternal cycle
        }
        if (size < 4) size = 4;
        *str = '-';
        str++;
        *str = 'i';
        str++;
        *str = 'n';
        str++;
        *str = 'f';
        str++;
        i += 4;
      }
    }
    while (i < specs.width) {
      *str = ' ';
      str++;
      i++;  // i am not sure about this string, but i think without it we
            // will have eternal cycle
    }
  }
  return str;
}

s21_size_t get_size_double(Spec *specs, long double num) {
  s21_size_t result = 0;       // returning result
  long double copy_num = num;  // do not want to change num and break it
  if (copy_num < 0)
    copy_num = -copy_num;  // reroll from - num if it is less than 0
  int aboba = (int)copy_num;
  copy_num -= aboba;
  while (aboba > 0) {
    aboba /= 10;
    result++;  // iterational changing of copy_num to count number of symbols in
               // it
  }
  result += (s21_size_t)specs->accuracy;
  if (specs->accuracy != 0 || specs->hash) result++;  // for .
  if (copy_num == 0 && result == 0 && (specs->width || specs->space)) result++;
  // if we have one of these specs, we need to add one place to symbol even
  // result = 0
  if (result < (s21_size_t)specs->width) result = specs->width;
  // if width is bigger we need to get a bigger result to make it equal to width
  // printf("GET_size-double %d, %d, %Lf\n", specs->space, specs->plus, num);
  if (specs->space || specs->plus) {
    specs->flag_size = 1;
    result++;
  }
  // if we have spec to have additional one place to string or num have '-' in
  // start of it we need to have additional place
  if (result == 0 && copy_num == 0 && !specs->accuracy && !specs->width &&
      !specs->space && !specs->dot)
    result++;
  //  if we still have result == 0 and other shit result should be 1 (for ex. we
  // had a number "0")
  if (result < 2 && copy_num == 0) result++;
  return result;
}

int unsigned_decimal_string(char *buf_str, Spec specs, unsigned long int num,
                            s21_size_t size_num) {
  int flag = 0, i = 0;
  if (specs.hash && specs.number_system == 8) specs.flag_size = 1;
  // review of # spec and number-system
  else if (specs.hash && specs.number_system == 16)
    specs.flag_size = 2;
  long int copy_num = num;
  i = decimal_string_zeros(specs, num, buf_str, size_num, i, copy_num, flag);
  return i;
}

int unsigned_decimal_string_helper(Spec specs, char *str_num,
                                   s21_size_t size_decimal, int i) {
  if (specs.hash && specs.number_system == 8) {
    str_num[i] = '0';
    i++;
    size_decimal--;
  } else if (specs.hash && specs.number_system == 16) {
    if (specs.upper_case)
      str_num[i] = 'X';
    else
      str_num[i] = 'x';
    i++;
    size_decimal--;
    str_num[i] = '0';
    i++;
    size_decimal--;
  }
  return i;
}

s21_size_t size_unsigned_decimal(Spec *specs, unsigned long int num) {
  s21_size_t result = 0;             // returning result
  unsigned long int copy_num = num;  // do not want to change num and break it
  while (copy_num > 0) {
    copy_num /= 10;
    result++;  // iterational changing of copy_num to count number of symbols in
               // it
  }
  if (copy_num == 0 && result == 0 &&
      (specs->accuracy || specs->width || specs->space))
    result++;
  // if we have one of these specs, we need to add one place to symbol even
  // result = 0
  if (result < (s21_size_t)specs->width) result = specs->width;
  // if width is bigger we need to get a bigger result to make it equal to width
  if (result < (s21_size_t)specs->accuracy) result = specs->accuracy;
  // analogical to accuracy
  if (specs->space || specs->plus) {
    specs->flag_size = 1;
    result++;
  }  // if we have spec to have additional one place to string or num have '-'
  // in start of it we need to have additional place
  if (result == 0 && copy_num == 0 && !specs->accuracy && !specs->width &&
      !specs->space && !specs->dot)
    result++;
  // if we still have result == 0 and other shit result should be 1 (for ex. we
  // had a number "0")
  return result;
}
