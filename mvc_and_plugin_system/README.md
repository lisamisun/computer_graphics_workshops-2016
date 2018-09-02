# Разделение логики и пользовательского интерфейса. Система плагинов.

Текст задания находится в файле *mvc_and_plugin_system.pdf*.

   Система: Qt Creator 3.5.1, Qt 5.5.1, g++ (Ubuntu 5.4.0-6ubuntu1~16.04.2) 5.4.0 20160609   
   ОС: Ubuntu 16.04 LTS   
   Аппаратура:   
- процессор: Intel® Core™ i3-2330M CPU @ 2.20GHz × 4 
- графика: Intel® Sandybridge Mobile 
- тип ОС - 64-разрядная
            
Пояснения к системе директорий:
  * ```Multitask_11/Task_1``` - тут лежат общие файлы и графический контроллер
  * ```Multitask_11/mainwindow``` - часть графического контроллера (вторая часть - в *main.cpp*)
  * ```Multitask_11/align_project``` - тут лежит код модели (собственно, машграф тут лежит), он общий для ```Task_1``` и ```Task_1_console```
  * ```Multitask_11/Task_1_console``` - тут прописан консольный контроллер
  * ```Multitask_11/build-...``` - содержит то, что собрал QtCreator

Без QtCreator'a может не собраться (а для QtCreator'а оставлены файлы _*.pro_).

Мультипроект собирался командой: **qmake Multitask_1.pro -r -spec linux-g++-64 CONFIG+=c++11**.

Плагины работают только в консольном режиме.

___

# Logic and user interface separation. Plugins system.

The text of the task is in *mvc_and_plugin_system.pdf*.
