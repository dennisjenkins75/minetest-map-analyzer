select node_x, node_y, node_z, a.name
from nodes l, actor a, node n
where (l.node_id = n.node_id)
  and (n.name = 'jumpdrive:engine')
  and (l.owner_id = a.actor_id)
order by a.name, l.pos_id;
