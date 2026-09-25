import matplotlib.pyplot as plt
import numpy as np

def plot_efficiency():
    # Probabilities from 0.0 to 0.5 as requested by the assignment
    p = np.linspace(0.0, 0.5, 50)
    
    # Constants
    a = 0.1 # Prop delay / Trans delay
    W = 4   # Window size for GBN and SR
    
    # Efficiency Formulas
    # Stop and Wait
    eta_saw = (1 - p) / (1 + 2*a)
    
    # Go-Back-N (Simplified for high error rate impact)
    eta_gbn = (W * (1 - p)) / ((1 + 2*a) * (1 - p + p * W))
    
    # Selective Repeat (Assuming W is large enough to keep pipe full)
    eta_sr = (W * (1 - p)) / (1 + 2*a)
    # Cap SR efficiency to 1.0 (or theoretical max without errors)
    eta_sr = np.minimum(eta_sr, 1 - p)
    
    # Plotting
    plt.figure(figsize=(10, 6))
    plt.plot(p, eta_saw, label='Stop-and-Wait', color='red', linestyle='--')
    plt.plot(p, eta_gbn, label=f'Go-Back-N (W={W})', color='blue', marker='o', markevery=5)
    plt.plot(p, eta_sr, label=f'Selective Repeat (W={W})', color='green', marker='s', markevery=5)
    
    plt.title('Throughput Efficiency vs Error/Loss Probability (p)')
    plt.xlabel('Probability of Packet Error / Loss (p)')
    plt.ylabel('Efficiency (\u03b7)')
    plt.legend()
    plt.grid(True)
    
    plt.tight_layout()
    plt.savefig('efficiency_comparison.png')
    print("Generated 'efficiency_comparison.png'")

if __name__ == "__main__":
    plot_efficiency()
