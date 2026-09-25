import pandas as pd
import matplotlib.pyplot as plt

def plot_metrics():
    # Load data
    df = pd.read_csv('simulation_results.csv')

    # Case 1: p-persistent varying p
    p_data = df[(df['Protocol'] == 'p-Persistent') & (df['Stations'] == 10)]
    
    if not p_data.empty:
        plt.figure(figsize=(15, 5))
        
        # Collisions
        plt.subplot(1, 3, 1)
        plt.plot(p_data['Probability'], p_data['Collisions'], marker='o', linestyle='-', color='r')
        plt.title('Collisions vs p (N=10)')
        plt.xlabel('Probability (p)')
        plt.ylabel('Total Collisions')
        plt.grid(True)
        
        # Delay
        plt.subplot(1, 3, 2)
        plt.plot(p_data['Probability'], p_data['AvgDelay'], marker='o', linestyle='-', color='b')
        plt.title('Transmission Delay vs p (N=10)')
        plt.xlabel('Probability (p)')
        plt.ylabel('Avg Delay (ms)')
        plt.grid(True)
        
        # Throughput
        plt.subplot(1, 3, 3)
        plt.plot(p_data['Probability'], p_data['Throughput'], marker='o', linestyle='-', color='g')
        plt.title('Throughput vs p (N=10)')
        plt.xlabel('Probability (p)')
        plt.ylabel('Normalized Throughput (S)')
        plt.grid(True)
        
        plt.tight_layout()
        plt.savefig('p_persistent_metrics.png')
        print("Generated 'p_persistent_metrics.png'")

    # Case 2: All schemes varying N
    protocols = ['Non-Persistent', '1-Persistent', 'p-Persistent_Opt', 'CSMA/CD']
    
    plt.figure(figsize=(15, 5))
    
    for proto in protocols:
        proto_data = df[(df['Protocol'] == proto)]
        if not proto_data.empty:
            plt.subplot(1, 3, 1)
            plt.plot(proto_data['Stations'], proto_data['Collisions'], marker='o', label=proto)
            
            plt.subplot(1, 3, 2)
            plt.plot(proto_data['Stations'], proto_data['AvgDelay'], marker='o', label=proto)
            
            plt.subplot(1, 3, 3)
            plt.plot(proto_data['Stations'], proto_data['Throughput'], marker='o', label=proto)

    plt.subplot(1, 3, 1)
    plt.title('Collisions vs Number of Stations (N)')
    plt.xlabel('Stations (N)')
    plt.ylabel('Total Collisions')
    plt.legend()
    plt.grid(True)
    
    plt.subplot(1, 3, 2)
    plt.title('Avg Delay vs Number of Stations (N)')
    plt.xlabel('Stations (N)')
    plt.ylabel('Avg Delay (ms)')
    plt.legend()
    plt.grid(True)
    
    plt.subplot(1, 3, 3)
    plt.title('Throughput vs Number of Stations (N)')
    plt.xlabel('Stations (N)')
    plt.ylabel('Normalized Throughput (S)')
    plt.legend()
    plt.grid(True)
    
    plt.tight_layout()
    plt.savefig('all_schemes_metrics.png')
    print("Generated 'all_schemes_metrics.png'")

if __name__ == "__main__":
    plot_metrics()
