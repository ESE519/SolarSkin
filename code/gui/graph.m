function graph(links,totalNodes,nodelabels,nodedesc,clear)

persistent adj h nodecolor

if nargin==5
    if(clear)
        h=[];
        adj=[];
    end
end
if isempty(adj) || size(adj,1)~=totalNodes
    adj = zeros(totalNodes);
end
for i=1:size(links,1)
    adj(links(i,1),links(i,2))=1;
end
        
nodecolor = hsv(totalNodes+1);
if isempty(h) 
    h = graphViz4Matlab('-adjMat', adj, '-layout', Treelayout,'-nodeColors',nodecolor,'-nodeLabels',nodelabels,'-nodeDescriptions',nodedesc);
else
    h.update('-adjMat', adj, '-layout', Treelayout,'-nodeColors',nodecolor,'-nodeLabels',nodelabels,'-nodeDescriptions',nodedesc);
end



end
    