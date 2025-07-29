#pragma once
#include <iostream>
#include <fstream>
#include <bitset>
#include <vector>
#include <string>
#include <sstream>
#define NEXT_DIV_BY_8(x)  ( ((x) + 7) & ~7 )
typedef char byte;
size_t gcd(size_t a, size_t b) {
    while (b != 0) {
        size_t t = b;
        b = a % b;
        a = t;
    }
    return a;
}
size_t lcm(size_t a, size_t b) {
    return (a / gcd(a, b)) * b;
}
void interleave(std::ifstream& in_stream, std::vector<std::ofstream>& out_streams, const size_t n, const size_t k) {

    size_t input_buffer_size, expected_read_bytes;
    input_buffer_size = expected_read_bytes = lcm((lcm(8, n * k) >> 3), k);
    size_t output_buffer_size = input_buffer_size / k;

    // Буфер чтения, в него считываем файл по input_buffer_size байт
    byte* input_buffer = new byte[input_buffer_size];
    // Буферы записи, всего k буферов для k файлов
    // Вместо двумерного указателя используем большой одномерный
    byte* output_buffer = new byte[output_buffer_size * k];

    // Буфер позиций для буферов записи; сохраняет число бит, записанных в каждый буфер
    size_t* out_bit_pos = new size_t[k];
    // Буфер позиций для буфера чтения; сохраняет позицию в буфере чтения для каждого буфера записи
    size_t* in_bit_pos = new size_t[k];
    memset(output_buffer, 0, output_buffer_size * k);

    for (size_t i = 0; i < k; i++) {
        // Инициализируем буфера позиций значениями по умолчанию
        *(out_bit_pos + i) = 0ull;
        *(in_bit_pos + i) = i * n;

    }
    while (true) {
        in_stream.read(input_buffer, expected_read_bytes);
        std::streamsize bytes_read = in_stream.gcount();
        if (bytes_read <= 0) {
            std::cout << "File ended" << '\n';
            break;
        }
        if (bytes_read < expected_read_bytes) {
            std::cout << "The last " << bytes_read << " bytes will be discarded.\n";
            break;
        }
        // Цикл по буферам записи
        for (size_t i = 0; i < k; i++) {
            // Число бит до конца секции чтения
            size_t bits_left_in_block = n;
            // Цикл по байтам внутри буфера записи
            for (size_t byte_idx = 0; byte_idx < output_buffer_size; byte_idx++) {
                byte assembled_byte = 0;
                // Число битов до конца байта буфера записи
                size_t bits_free_in_output_byte = 8;
                // Сшиваем байт b
                while (true) {
                    // Число битов до конца байта буфера чтения
                    size_t bits_till_end_of_input_byte;
                    if ((in_bit_pos[i] & 7) == 0) {
                        bits_till_end_of_input_byte = 8;
                    }
                    else {
                        bits_till_end_of_input_byte = NEXT_DIV_BY_8(in_bit_pos[i]) - in_bit_pos[i];
                    }
                    // Получаем байт из буфера чтения
                    byte curr_read_byte = input_buffer[in_bit_pos[i] / 8];
                    /* Сшивка байта*/
                    // Число прочитанных бит
                    size_t bits_to_copy = std::min({
                                            bits_left_in_block,
                                            bits_free_in_output_byte,
                                            bits_till_end_of_input_byte
                        });

                    // Случай, когда читаем целый байт из буфера чтения
                    if (bits_to_copy == 8ull)
                        assembled_byte = curr_read_byte;
                    else {
                        unsigned char bit_mask = (1u << 8) - (1u << (8 - bits_to_copy));
                        //std::cout << std::bitset<8>(mask) << '\n';
                        //std::cout << std::bitset<8>(read_byte) << '\n';
                        curr_read_byte = _rotl8(curr_read_byte, in_bit_pos[i] & 7);
                        //std::cout << std::bitset<8>(read_byte) << '\n';
                        curr_read_byte &= bit_mask;
                        //std::cout << std::bitset<8>(read_byte) << '\n';
                        curr_read_byte = _rotr8(curr_read_byte, out_bit_pos[i]);
                        //std::cout << std::bitset<8>(read_byte) << '\n';
                        //std::cout << std::bitset<8>(b) << '\n';
                        assembled_byte |= curr_read_byte;
                        //std::cout << std::bitset<8>(b) << '\n';
                        out_bit_pos[i] += bits_to_copy;
                    }

                    bits_left_in_block -= bits_to_copy;
                    bits_free_in_output_byte -= bits_to_copy;
                    bits_till_end_of_input_byte -= bits_to_copy;

                    size_t read_offset = bits_to_copy;
                    // Если дошли до конца секции чтения, то переходим в следующую
                    if (bits_left_in_block == 0) {
                        read_offset = n * k;
                        bits_left_in_block = n;
                    }
                    in_bit_pos[i] += read_offset;
                    if (in_bit_pos[i] >= input_buffer_size << 3)
                        in_bit_pos[i] %= input_buffer_size << 3;
                    // Если сшили байт, прерываем цикл
                    if (bits_free_in_output_byte == 0) {
                        out_bit_pos[i] = 0ull;
                        break;
                    }


                }
                output_buffer[output_buffer_size * i + byte_idx] = assembled_byte;
            }
            out_streams[i].write(output_buffer + output_buffer_size * i, output_buffer_size);
            memset(output_buffer + output_buffer_size * i, 0, output_buffer_size);
        }
    }
    delete[] input_buffer;
    delete[] in_bit_pos;
    delete[] output_buffer;
    delete[] out_bit_pos;
}

// Версия для гугл-тестов
void interleave(std::istringstream& in_stream, std::vector<std::ostringstream>& out_streams, const size_t n, const size_t k) {
    size_t input_buffer_size, expected_read_bytes;
    input_buffer_size = expected_read_bytes = lcm((lcm(8, n * k) >> 3), k);
    size_t output_buffer_size = input_buffer_size / k;
    size_t block_offset = n * (k - 1);
    // Буфер чтения, в него считываем файл по input_buffer_size байт
    byte* input_buffer = new byte[input_buffer_size];
    // Буферы записи, всего k буферов для k файлов
    // Вместо двумерного указателя используем большой одномерный
    byte* output_buffer = new byte[output_buffer_size * k];

    // Буфер позиций для буферов записи; сохраняет число бит, записанных в каждый буфер
    size_t* out_bit_pos = new size_t[k];
    // Буфер позиций для буфера чтения; сохраняет позицию в буфере чтения для каждого буфера записи
    size_t* in_bit_pos = new size_t[k];
    memset(output_buffer, 0, output_buffer_size * k);

    for (size_t i = 0; i < k; i++) {
        // Инициализируем буфера позиций значениями по умолчанию
        *(out_bit_pos + i) = 0ull;
        *(in_bit_pos + i) = i * n;

    }
    while (true) {
        in_stream.read(input_buffer, expected_read_bytes);
        std::streamsize bytes_read = in_stream.gcount();
        if (bytes_read <= 0) {
            std::cout << "File ended" << '\n';
            break;
        }
        if (bytes_read < expected_read_bytes) {
            std::cout << "The last " << bytes_read << " bytes will be discarded.\n";
            break;
        }
        // Цикл по буферам записи
        for (size_t i = 0; i < k; i++) {
            // Число бит до конца секции чтения
            size_t bits_left_in_block = n;
            // Цикл по байтам внутри буфера записи
            for (size_t byte_idx = 0; byte_idx < output_buffer_size; byte_idx++) {
                byte assembled_byte = 0;
                // Число битов до конца байта буфера записи
                size_t bits_free_in_output_byte = 8;
                // Сшиваем байт b
                while (true) {
                    // Число битов до конца байта буфера чтения
                    size_t bits_till_end_of_input_byte;
                    if ((in_bit_pos[i] & 7) == 0) {
                        bits_till_end_of_input_byte = 8;
                    }
                    else {
                        bits_till_end_of_input_byte = NEXT_DIV_BY_8(in_bit_pos[i]) - in_bit_pos[i];
                    }
                    // Получаем байт из буфера чтения
                    byte curr_read_byte = input_buffer[in_bit_pos[i] >> 3];
                    /* Сшивка байта*/
                    // Число прочитанных бит
                    size_t bits_to_copy = std::min({
                                            bits_left_in_block,
                                            bits_free_in_output_byte,
                                            bits_till_end_of_input_byte
                        });

                    // Случай, когда читаем целый байт из буфера чтения
                    if (bits_to_copy == 8ull)
                        assembled_byte = curr_read_byte;
                    else {
                        unsigned char bit_mask = (1u << 8) - (1u << (8 - bits_to_copy));
                        curr_read_byte = _rotl8(curr_read_byte, in_bit_pos[i] & 7);
                        curr_read_byte &= bit_mask;
                        curr_read_byte = _rotr8(curr_read_byte, out_bit_pos[i]);
                        assembled_byte |= curr_read_byte;
                        out_bit_pos[i] += bits_to_copy;
                    }

                    bits_left_in_block -= bits_to_copy;
                    bits_free_in_output_byte -= bits_to_copy;

                    size_t read_offset = bits_to_copy;
                    // Если дошли до конца секции чтения, то переходим в следующую
                    if (bits_left_in_block == 0) {
                        read_offset += block_offset;
                        bits_left_in_block = n;
                    }
                    in_bit_pos[i] += read_offset;
                    if (in_bit_pos[i] >= input_buffer_size << 3)
                        in_bit_pos[i] %= input_buffer_size << 3;
                    // Если сшили байт, прерываем цикл
                    if (bits_free_in_output_byte == 0) {
                        out_bit_pos[i] = 0ull;
                        break;
                    }


                }
                output_buffer[output_buffer_size * i + byte_idx] = assembled_byte;
            }
            out_streams[i].write(output_buffer + output_buffer_size * i, output_buffer_size);
            memset(output_buffer + output_buffer_size * i, 0, output_buffer_size);
        }
    }
    delete[] input_buffer;
    delete[] in_bit_pos;
    delete[] output_buffer;
    delete[] out_bit_pos;
}


