import subprocess
import re
import sys
import os
import time
import argparse
from datetime import datetime
from typing import Dict, Optional

class VeriSimplePIRRunner:
    def __init__(self, base_dir, N: int = 20, d: int = 64):
        self.base_dir = os.path.abspath(base_dir)
        self.lib_path = os.path.join(self.base_dir, "bin/lib")
        self.env = os.environ.copy()
        self.env["LD_LIBRARY_PATH"] = f"{self.lib_path}:{self.env.get('LD_LIBRARY_PATH', '')}"
        self.N = N
        self.d = d

    def run_command(self, cmd, ignore_errors=False):
        full_cmd = os.path.join(self.base_dir, cmd)
        try:
            result = subprocess.run(
                full_cmd, 
                capture_output=True, 
                text=True,
                env=self.env,
                cwd=self.base_dir 
            )
            # When ignore_errors is True, return stdout even if command failed
            if ignore_errors or result.returncode == 0:
                return result.stdout
            else:
                print(f"Command failed with error: {result.stderr}")
                return None
        except Exception as e:
            print(f"Error running command: {e}")
            return None

    def extract_basis(self, output):
        if not output:
            return None
        match = re.search(r'p = \d+ = 2\^(\d+)', output)
        if match:
            return int(match.group(1))
        return None

    def update_basis_in_files(self, basis):
        file_path = os.path.join(self.base_dir, "src/demo/global_parameters.h")
        
        if not os.path.exists(file_path):
            print(f"Cannot find parameters file at {file_path}")
            return False
            
        try:
            with open(file_path, 'r') as f:
                content = f.read()
            
            # Update BASIS definition
            content = re.sub(r'#define\s+BASIS_VALUE\s+\d+', f'#define BASIS_VALUE {basis}', content)
            
            with open(file_path, 'w') as f:
                f.write(content)
            return True
        except Exception as e:
            print(f"Error updating basis: {e}")
            return False

    def compile_project(self):
        try:
            subprocess.run(["make"], cwd=self.base_dir, check=True)
            return True
        except subprocess.CalledProcessError as e:
            print(f"Compilation failed: {e}")
            return False

    def run_and_collect_metrics(self) -> Optional[Dict[str, float]]:
        # Update N and d parameters first
        if not self.update_parameters_in_files():
            return None
            
        if not self.compile_project():
            return None
        # First run to get basis
        print("Running initial preproc_pir_bench to get basis...")
        output = self.run_command("bin/demo/bench/preproc_pir_bench", ignore_errors=True)
        if not output:
            print("Failed to get any output from preproc_pir_bench")
            return None

        basis = self.extract_basis(output)
        if not basis:
            print("Could not extract basis value")
            return None

        print(f"Found basis: {basis}")
        
        # Update basis and recompile
        if not self.update_basis_in_files(basis):
            return None
        
        if not self.compile_project():
            return None

        # Run both scripts
        print("Running preproc_pir_bench...")
        preproc_output = self.run_command("bin/demo/bench/preproc_pir_bench")
        
        print("Running params script...")
        params_output = self.run_command("bin/demo/scripts/params")

        return self.parse_outputs(preproc_output, params_output)

    def update_parameters_in_files(self) -> bool:
        file_path = os.path.join(self.base_dir, "src/demo/global_parameters.h")
        
        if not os.path.exists(file_path):
            print(f"Cannot find parameters file at {file_path}")
            return False
            
        try:
            with open(file_path, 'r') as f:
                content = f.read()
            
            # Update N and d values
            content = re.sub(r'#define\s+N_VALUE\s+\d+', f'#define N_VALUE {self.N}', content)
            content = re.sub(r'#define\s+D_VALUE\s+\d+', f'#define D_VALUE {self.d}', content)
            
            with open(file_path, 'w') as f:
                f.write(content)
            return True
        except Exception as e:
            print(f"Error updating parameters: {e}")
            return False
            
    def parse_outputs(self, preproc_output, params_output):
        if not preproc_output or not params_output:
            return None
            
        metrics = {}

        # Save logs with timestamp
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        log_filename = f"run_log_N{self.N}_d{self.d}_{timestamp}.txt"

        # Create metrics/logs directory if it doesn't exist
        log_dir = os.path.join(self.base_dir, "metrics/logs")
        os.makedirs(log_dir, exist_ok=True)
    
        # Save full output
        with open(os.path.join(log_dir, log_filename), 'w') as f:
            f.write("=== preproc_pir_bench output ===\n")
            f.write(preproc_output)
            f.write("\n\n=== params output ===\n")
            f.write(params_output)
        
        print(f"Full run log saved to {log_filename}")
            
        # Parse from preproc_pir_bench output
        preproc_patterns = {
            'A Expansion Time (ms)': r'A expansion time: ([\d.]+) ms',  # Added this pattern
            'Global Prepr (s)': r'Global preprocessing [^:]+: ([\d.]+) ms',
            'Server Per-Client Prepr (s)': r'Server Per-Client Prepr \(s\) : ([\d.]+)',
            'Client Local Prepr (s)': r'Client Local Prepr \(s\) : ([\d.]+)',
            'Client Prepa Pre Req (s)': r'Client Prepa Pre Req \(s\) : ([\d.]+)',
            'Server Prepa Comp (s)': r'Server Prepa Comp \(s\) : ([\d.]+)',
            'Client Prepa Post Req (s)': r'Client Prepa Post Req \(s\) : ([\d.]+)',
            'Query: Client Req Gen (ms)': r'Query: Client Req Gen \(ms\) : ([\d.]+)',
            'Query: Server Comp (s)': r'Query: Server Comp \(s\) : ([\d.]+)',
            'Query: Client Decryption (ms)': r'Query: Client Decryption \(ms\) : ([\d.]+)',
            'Query: Client Verification (ms)': r'Query: Client Verification \(ms\) : ([\d.]+)',
            'VSPIR Uncompr. Z (KiB)': r'VSPIR Uncompr\. Z \(KiB\) : ([\d.]+)'
        }
    
        # Parse from params output
        params_patterns = {
            'Hints (MiB)': r'Hints \(MiB\): hint download = ([\d.]+)',
            'Online State (KiB)': r'Online State \(KiB\): client storage = ([\d.]+)',
            'Offline Up (KiB)': r'Offline Up \(KiB\): preproc upload = ([\d.]+)',
            'Offline Down (KiB)': r'Offline Down \(KiB\): preproc download  = ([\d.]+)',
            'Query Up (KiB)': r'Query Up \(KiB\): online upload size = ([\d.]+)',
            'Query Down (KiB)': r'Query Down \(KiB\): online download size = ([\d.]+)'
        }
    
        # Save parsed metrics
        metrics_filename = f"parsed_metrics_N{self.N}_d{self.d}_{timestamp}.txt"
    
        for metric, pattern in preproc_patterns.items():
            match = re.search(pattern, preproc_output)
            if match:
                metrics[metric] = float(match.group(1))
    
        for metric, pattern in params_patterns.items():
            match = re.search(pattern, params_output)
            if match:
                metrics[metric] = float(match.group(1))
            
        # Save parsed metrics
        with open(os.path.join(log_dir, metrics_filename), 'w') as f:
            f.write("Parsed Metrics:\n")
            for metric, value in metrics.items():
                f.write(f"{metric}: {value}\n")
            
        print(f"Parsed metrics saved to {metrics_filename}")
            
        return metrics

def batch_run(base_dir: str, configs: list) -> list:
    """Run multiple experiments with different configurations"""
    results = []
    
    for config in configs:
        print(f"\nRunning experiment with N={config['N']}, d={config['d']}")
        runner = VeriSimplePIRRunner(base_dir, config['N'], config['d'])
        metrics = runner.run_and_collect_metrics()
        if metrics:
            metrics['N'] = config['N']
            metrics['d'] = config['d']
            results.append(metrics)
        else:
            print(f"Failed to collect metrics for N={config['N']}, d={config['d']}")
    
    return results

def main():
    parser = argparse.ArgumentParser(description='Run VeriSimplePIR experiments')
    parser.add_argument('--N', type=int, default=1048576, help='Database size')
    parser.add_argument('--d', type=int, default=64, help='Record size')
    parser.add_argument('--batch', action='store_true', help='Run batch experiments')
    args = parser.parse_args()

    verisimplepir_dir = os.path.expanduser("~/pir/VeriSimplePIR")
    
    if not os.path.exists(verisimplepir_dir):
        print(f"VeriSimplePIR directory not found at {verisimplepir_dir}")
        sys.exit(1)

    if args.batch:
        # Define batch configurations
        configs = [
            {'N': 20, 'd': 64},    # 2^20, 8B
            {'N': 20, 'd': 2048},  # 2^20, 256B
            {'N': 26, 'd': 64}, 
            {'N': 30, 'd': 8}, 
            {'N': 18, 'd': 262144}
        ]
        
        results = batch_run(verisimplepir_dir, configs)
        
        print("\nBatch Results:")
        for result in results:
            print(f"\nN={result['N']}, d={result['d']}")
            for metric, value in result.items():
                if metric not in ['N', 'd']:
                    print(f"{metric}: {value}")
    else:
        # Single run with command line parameters
        runner = VeriSimplePIRRunner(verisimplepir_dir, args.N, args.d)
        metrics = runner.run_and_collect_metrics()
        
        if metrics:
            print("\nCollected Metrics:")
            for metric, value in metrics.items():
                print(f"{metric}: {value}")
        else:
            print("Failed to collect metrics")

if __name__ == "__main__":
    main()