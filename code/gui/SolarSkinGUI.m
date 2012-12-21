function SolarSkinGUI()
global nodeList nodeDesc
s = serial('/dev/tty.usbmodem1412', 'BaudRate', 9600, 'DataBits', 8, 'Parity', 'none', 'StopBits', 1, 'FlowControl', 'none','Terminator','CR/LF');
cleanupObj = onCleanup(@()fclose(s));
s.BytesAvailableFcnMode = 'Terminator';
s.BytesAvailableFcn = @parseData;
s.timeout=100000;
nodeList = [1];
nodeDesc = {'Gateway'};
%heat_map=zeros(120,120,3);
fopen(s);
while 1
    fprintf('.');
    pause(2);
end
end

function parseData(obj,event) 
    global nodeList nodeDesc
    [data] = fread(obj,obj.BytesAvailable);
    data=double(data);
    node=data(1);
    n = numel(data)-6;
    links = zeros(n-1,2);
    for i=1:n-1
        links(i,:) = [find(nodeList==data(i)) find(nodeList==data(i+1))];
    end
    
    if ~any(ismember(nodeList,node))
        nodeList = [nodeList node];
    end
    [Temp Light]=calibration_compute((data(end-5)*256)+data(end-4),(data(end-3)*256)+data(end-2));
    nodeDesc(nodeList==node) = {sprintf('Temperature: %d, Light: %d',Temp,Light)}; 
    nl ={};
    for i=1:numel(nodeList)
        nl(i)={num2str(nodeList(i))};
    end
    
    graph(links, numel(nodeList),nl.',nodeDesc.');
    my_data = [node Light Temp];
    fid = fopen('readings.txt','a');
    c = fwrite(fid, my_data);
    c = fwrite(fid,'\n');
    fclose(fid);
end

