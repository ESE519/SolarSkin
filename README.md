Project SolarSkin
Team: Amey Pendharkar, Miraj Shah, Neel S Shah

Final Video Links

1. http://www.youtube.com/watch?v=wjxvmUaf5Q0

2. http://www.youtube.com/watch?v=PzuQBJTAVmc


Getting Started


1. Gateway code is under the gateway directory. (Keil project)

2. Node code is under the node directory. (Keil Project) 

3. Nodes should be loaded such that every node has a different address. Change the address in the mainNode.cpp file. #define MyOwnAddress is the one that needs to be changed.

4. Install graphViz 2.8 from this link http://www.graphviz.org/

5. Add the appropriate paths to the matlab path. 

For MAC, if graphviz is installed using brew, run this in matlab

setenv('PATH',[getenv('PATH'),pathsep(),'/usr/local/bin');

For Windows, with default directory  

setenv('PATH',[getenv('PATH'),pathsep(),'C:/Program Files/graphviz 2.8/bin');

6. Add paths for the folder gui and all subfolders to the matlab search.

7. Update the appropriate serial port in the SolarSkinGUI.m file 

8. Run SolarSkinGUI in matlab.



See the dynamic update of the connectivity graph of the network. 

To see the temperature and light readings just double click the corresponding node and it will show the readings. 
 
 
 