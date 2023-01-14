select count(1), a.name as name
from actor a, node n, nodes l
where (a.actor_id = l.owner_id)
  and (n.node_id = l.node_id)
  and (n.name = 'bones:bones')
group by a.name
order by 1 desc
limit 5;
