// Light culling compute shader for forward+ clustered shading
// Groups lights into clusters for efficient fragment shader lighting

#define MAX_LIGHTS_PER_CLUSTER 64
#define CLUSTER_SIZE_X 8
#define CLUSTER_SIZE_Y 8
#define CLUSTER_SIZE_Z 8

struct Light {
    float3 position;
    float radius;
    float3 color;
    float intensity;
    float3 direction;
    uint type;        // 0 = point, 1 = spot, 2 = directional
    float inner_cone;
    float outer_cone;
    uint padding;
};

struct ClusterAABB {
    float3 min_bounds;
    uint padding1;
    float3 max_bounds;
    uint padding2;
};

struct ClusterData {
    uint offset;
    uint count;
};

// Input bindings
RWStructuredBuffer<ClusterData> LightGrid : register(u0);
StructuredBuffer<Light> Lights : register(t0);
StructuredBuffer<ClusterAABB> ClusterAABBs : register(t1);
RWStructuredBuffer<uint> LightIndexList : register(u2);

// Cluster parameters
cbuffer ClusterParams : register(b0) {
    float4x4 ViewProjection;
    float4x4 InverseViewProjection;
    float3 CameraPosition;
    uint ClusterCountX;
    uint ClusterCountY;
    uint ClusterCountZ;
    float3 ClusterScale;
    float3 ClusterBias;
    uint LightCount;
    float2 ScreenDimensions;
    float NearPlane;
    float FarPlane;
};

// Shared memory for light indices within thread group
groupshared uint SharedLightIndices[MAX_LIGHTS_PER_CLUSTER];
groupshared uint SharedLightCount;

// Helper functions
bool SphereIntersectsAABB(Light light, ClusterAABB aabb) {
    float3 closest_point = {
        max(aabb.min_bounds.x, min(light.position.x, aabb.max_bounds.x)),
        max(aabb.min_bounds.y, min(light.position.y, aabb.max_bounds.y)),
        max(aabb.min_bounds.z, min(light.position.z, aabb.max_bounds.z))
    };
    
    float3 distance_vec = light.position - closest_point;
    float distance_squared = dot(distance_vec, distance_vec);
    
    return distance_squared <= (light.radius * light.radius);
}

bool SpotIntersectsAABB(Light light, ClusterAABB aabb) {
    // First test sphere intersection
    if (!SphereIntersectsAABB(light, aabb)) {
        return false;
    }
    
    // Test cone intersection
    float3 cluster_center = (aabb.min_bounds + aabb.max_bounds) * 0.5f;
    float3 light_to_cluster = cluster_center - light.position;
    float distance = length(light_to_cluster);
    
    if (distance > light.radius) {
        return false;
    }
    
    float3 light_to_cluster_normalized = light_to_cluster / distance;
    float cos_angle = dot(light.direction, light_to_cluster_normalized);
    
    return cos_angle >= light.outer_cone;
}

uint GetClusterIndex(uint3 thread_id) {
    return thread_id.z * ClusterCountX * ClusterCountY + 
           thread_id.y * ClusterCountX + 
           thread_id.x;
}

[numthreads(CLUSTER_SIZE_X, CLUSTER_SIZE_Y, CLUSTER_SIZE_Z)]
void main(uint3 dispatch_thread_id : SV_DispatchThreadID,
           uint3 group_thread_id : SV_GroupThreadID,
           uint3 group_id : SV_GroupID) {
    
    // Initialize shared memory
    if (group_thread_id.x == 0 && group_thread_id.y == 0 && group_thread_id.z == 0) {
        SharedLightCount = 0;
    }
    GroupMemoryBarrierWithGroupSync();
    
    // Check if this thread is within bounds
    if (dispatch_thread_id.x >= ClusterCountX || 
        dispatch_thread_id.y >= ClusterCountY || 
        dispatch_thread_id.z >= ClusterCountZ) {
        return;
    }
    
    uint cluster_index = GetClusterIndex(dispatch_thread_id);
    ClusterAABB cluster_aabb = ClusterAABBs[cluster_index];
    
    // Test each light against this cluster
    for (uint light_index = 0; light_index < LightCount; light_index++) {
        Light light = Lights[light_index];
        
        bool intersects = false;
        switch (light.type) {
            case 0: // Point light
                intersects = SphereIntersectsAABB(light, cluster_aabb);
                break;
            case 1: // Spot light
                intersects = SpotIntersectsAABB(light, cluster_aabb);
                break;
            case 2: // Directional light
                // Directional lights affect all clusters
                intersects = true;
                break;
        }
        
        if (intersects) {
            // Add light to shared memory atomically
            uint current_count;
            InterlockedAdd(SharedLightCount, 1, current_count);
            
            if (current_count < MAX_LIGHTS_PER_CLUSTER) {
                SharedLightIndices[current_count] = light_index;
            }
        }
    }
    
    GroupMemoryBarrierWithGroupSync();
    
    // Write results to global memory
    uint start_offset;
    InterlockedAdd(LightGrid[0].offset, SharedLightCount, start_offset);
    
    // Clamp light count to maximum per cluster
    uint light_count = min(SharedLightCount, MAX_LIGHTS_PER_CLUSTER);
    
    // Write cluster data
    LightGrid[cluster_index].offset = start_offset;
    LightGrid[cluster_index].count = light_count;
    
    // Write light indices
    for (uint i = 0; i < light_count; i++) {
        LightIndexList[start_offset + i] = SharedLightIndices[i];
    }
}
