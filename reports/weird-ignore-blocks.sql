select b.mapblock_x * 16, b.mapblock_y * 16, b.mapblock_z * 16, n.name
from blocks b, node n
where (b.uniform = n.node_id)
  and (n.name = 'ignore')
  and (b.mapblock_x * 16 between -29000 and 29000)
  and (b.mapblock_y * 16 between 0 and 29000)
  and (b.mapblock_z * 16 between -29000 and 29000)
order by b.mapblock_y asc
limit 30;

