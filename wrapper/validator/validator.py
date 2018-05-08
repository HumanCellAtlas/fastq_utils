import subprocess
from common.validation_report import ValidationReport


class Validator:

    def validate(self, file_path):
        report = ValidationReport()

        process = subprocess.Popen(["fastq_info", file_path, "-r", "-s"], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        process.wait()
        err_lines = process.stderr.readlines()
        for line in err_lines:
            line_str = line.decode("utf-8")
            if "ERROR" in line_str:
                report.log_error(line_str.rstrip())

        report.state = "INVALID" if report.errors else "VALID"

        return report
