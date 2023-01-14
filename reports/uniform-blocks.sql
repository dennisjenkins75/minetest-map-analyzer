select count(1), n.name
from blocks b, node n
where (n.node_id = b.uniform)
group by 2
order by 1 desc;
