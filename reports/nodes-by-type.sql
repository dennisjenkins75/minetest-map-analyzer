select n.name, count(1)
from node n, nodes l
where (n.node_id = l.node_id)
group by n.name
order by 2 desc;
