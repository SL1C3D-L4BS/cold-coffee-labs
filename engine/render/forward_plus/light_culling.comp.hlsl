// Light culling compute shader for Forward+ clustered lighting
// Assigns lights to clusters for efficient forward rendering

cbuffer ClusterConstants : register(b0)
{
    uint3 ClusterCount;      // x, y, z cluster dimensions
    uint LightCount;
    float4x4 InvProjection;
    float4x4 InvView;
    float2 ScreenSize;
    float ZNear;
    float ZFar;
    float FovY;
};

struct Light
{
    float3 Position;
    float3 Color;
    float Intensity;
    float Radius;
    uint Type;              // 0 = point, 1 = spot, 2 = directional
    float3 Direction;
    float InnerCone;
    float OuterCone;
};

struct ClusterAABB
{
    float3 MinBounds;
    float3 MaxBounds;
};

// Input buffers
StructuredBuffer<Light> Lights : register(t0);
StructuredBuffer<ClusterAABB> ClusterAABBs : register(t1);

// Output buffers
RWStructuredBuffer<uint> LightGrid : register(u0);     // 2 values per cluster: offset, count
RWStructuredBuffer<uint> LightList : register(u1);     // Light indices

// Helper functions
float LinearDepth(float depth)
{
    return (ZNear * ZFar) / (ZFar - depth * (ZFar - ZNear));
}

float3 ScreenToViewSpace(float2 screenPos, float depth)
{
    float4 clipPos = float4(screenPos * 2.0 - 1.0, depth, 1.0);
    float4 viewPos = mul(InvProjection, clipPos);
    return viewPos.xyz / viewPos.w;
}

bool SphereIntersectsAABB(float3 sphereCenter, float sphereRadius, float3 aabbMin, float3 aabbMax)
{
    float3 closestPoint = clamp(sphereCenter, aabbMin, aabbMax);
    float3 distanceVec = sphereCenter - closestPoint;
    return dot(distanceVec, distanceVec) <= (sphereRadius * sphereRadius);
}

bool SpotIntersectsAABB(Light light, float3 aabbMin, float3 aabbMax)
{
    // First test sphere intersection
    if (!SphereIntersectsAABB(light.Position, light.Radius, aabbMin, aabbMax))
        return false;
    
    // Then test cone intersection (simplified)
    float3 clusterCenter = (aabbMin + aabbMax) * 0.5;
    float3 lightToCluster = clusterCenter - light.Position;
    float distance = length(lightToCluster);
    
    if (distance > light.Radius)
        return false;
    
    float3 lightToClusterNormalized = lightToCluster / distance;
    float cosAngle = dot(light.Direction, lightToClusterNormalized);
    
    return cosAngle >= light.OuterCone;
}

[numthreads(8, 8, 8)]
void CSMain(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint3 clusterIndex = dispatchThreadID;
    
    // Check if we're within cluster bounds
    if (any(clusterIndex >= ClusterCount))
        return;
    
    uint flatClusterIndex = clusterIndex.z * ClusterCount.x * ClusterCount.y +
                           clusterIndex.y * ClusterCount.x +
                           clusterIndex.x;
    
    // Get cluster AABB
    ClusterAABB cluster = ClusterAABBs[flatClusterIndex];
    
    // Count lights that intersect this cluster
    uint lightCount = 0;
    uint startOffset = 0;
    
    // First pass: count lights
    for (uint i = 0; i < LightCount; ++i)
    {
        Light light = Lights[i];
        bool intersects = false;
        
        switch (light.Type)
        {
            case 0: // Point light
                intersects = SphereIntersectsAABB(light.Position, light.Radius, 
                                               cluster.MinBounds, cluster.MaxBounds);
                break;
            case 1: // Spot light
                intersects = SpotIntersectsAABB(light, cluster.MinBounds, cluster.MaxBounds);
                break;
            case 2: // Directional light
                // Directional lights affect all clusters
                intersects = true;
                break;
        }
        
        if (intersects)
            lightCount++;
    }
    
    // Allocate space in light list (simplified - would need atomic operations in real implementation)
    startOffset = flatClusterIndex * 8; // Max 8 lights per cluster
    
    // Second pass: add light indices
    uint currentLight = 0;
    for (uint i = 0; i < LightCount && currentLight < 8; ++i)
    {
        Light light = Lights[i];
        bool intersects = false;
        
        switch (light.Type)
        {
            case 0: // Point light
                intersects = SphereIntersectsAABB(light.Position, light.Radius, 
                                               cluster.MinBounds, cluster.MaxBounds);
                break;
            case 1: // Spot light
                intersects = SpotIntersectsAABB(light, cluster.MinBounds, cluster.MaxBounds);
                break;
            case 2: // Directional light
                intersects = true;
                break;
        }
        
        if (intersects)
        {
            LightList[startOffset + currentLight] = i;
            currentLight++;
        }
    }
    
    // Write cluster data
    LightGrid[flatClusterIndex * 2] = startOffset;     // offset
    LightGrid[flatClusterIndex * 2 + 1] = lightCount; // count
}
