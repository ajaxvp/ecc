/* ISO: 6.6 (4); value bound checks in constant expressions */

int a = 0x7FFFFFFF + 1;

int b = ((int) 0x80000000) - 1;

int c = 0x7FFFFFFF * 2;
