# d3d12-experiments
Resource Management in D3D12

```mermaid
block-beta
columns 2
desc_label(["Descriptors/Descriptor Heaps"]):1
res_label(["Resources"]):1

dummy_descriptor_for_shift["..."]:1

ds_1["Depth/Stencil Buffer"]:1
block
	columns 2
	dsv_1["DSV1"] dsv_n["..."]
end

rt_1["Render Target Buffer"]:1
block
    columns 2
	rtv_1["RTV1"] rtv_n["..."]
end

ua_1["Unordered Access Buffer"]:1
uav_1["UAV"]:1

cb_1["Constant Buffer"]:1
cbv_1["CBV"]:1

sr_1["Shader Resource"]:1
srv_1["SRV"]:1

dummy_resource_for_offset["..."]:1

dsv_1-->ds_1
rtv_1-->rt_1
uav_1-->ua_1
cbv_1-->cb_1
srv_1-->sr_1
```
