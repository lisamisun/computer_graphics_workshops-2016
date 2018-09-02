# Векторная обработка данных. Документирование кода. Модульное тестирование.

Текст задания находится в файле *vector_processing.pdf*.

   Система: Qt Creator 3.5.1, Qt 5.5.1, g++ (Ubuntu 5.4.0-6ubuntu1~16.04.2) 5.4.0 20160609  
   ОС: Ubuntu 16.04 LTS  
   Аппаратура:  
- процессор: Intel® Core™ i3-2330M CPU @ 2.20GHz × 4 
- графика: Intel® Sandybridge Mobile 
- тип ОС - 64-разрядная

Пояснения к системе директорий: оставила проект так, как было в машграфе. 
  * ```template/include```: заголовки файлов для использования в проекте, сюда добавлена *Timer.h* (немного переделана, остались только функции ```start()```, ```stop()```, ```restart()``` и ```elapsed_time()```, которая вынесена в public); ещё тут лежит *lennatest.h* - заголовочный файл для *lennatest.cpp* из папки ```src```.
  * ```build/bin```: отсюда можно запустить проект командой: **./task_2 <путь до файла, содержащего пути до всех изображений и "ярлыки" (типа номера класса) к ним>**;
  * ```doc/html/index.html```: так открывается документация;
  * ```src```: код программы(*task_2.cpp*);
  * ```src_gtest```: код для *GoogleTest* (*lennatest.cpp*);
      ```externals/googletest```: клонированная с гитхаба папка для работы с *GoogleTest*.
   Остальные папки содержат всякие внешние библиотеки для правильной сборки и работы программы.  

   Проект собирается из папки ```temlate``` командой **make all** из папки ```template``` (можно сделать **make clean**, а потом **make all**; но вообще я оставила все бинарники). Документация генерируется вместе с бинарниками проекта и лежит в папке doc.  
   *GoogleTest* собирается и запускается командой **make test** из папки ```template```.  

Теперь комментарий по поводу времени работы функций с SSE и без него. Я прогоняла программу на всех тестовых картинках машграфа командой: **./task2 ../../../data/multiclass/train_labels.txt** и считала сумму работы функций с использованием SSE и без оного на этих картинках. Получаются следующие результаты:

- свёртка с фильтром Собеля для компонеты x: 
      Time of Sobel_x: 0.048 seconds    
      Time of Sobel_x_sse: 0.047 seconds    
- свёртка с фильтром Собеля для компонеты y: 
      Time of Sobel_y: 0.047 seconds    
      Time of Sobel_y_sse: 0.047 seconds    
- рассчёт модуля градиента:
      Time of Absolute: 0.033 seconds    
      Time of Absolute_sse: 0.020 seconds    


Можно сказать (хотя и с натяжкой), что SSE убыстряет работу функций, однако результаты отличаются не очень сильно, потому что:
- в ```Makefile``` машграфа проект собирается с опцией *-O2* оптимизации, а компилятор умеет хорошо оптимизирвать;
- загрузки данных в переменные типа ```__m128``` и выгрузки их оттуда занимают много времени.
Таким образом, хотя бы на последней функции SSE значительно убыстряет работу за счёт того, что в каждый момент времени считаются одновременно несколько (4, в данном случае) значения, а не одно, как без SSE.

___

# Vector data processing. Code documenting. Unit testing.

The text of the task is in *vector_processing.pdf*.




        
