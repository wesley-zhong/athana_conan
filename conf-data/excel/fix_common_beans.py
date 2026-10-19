import openpyxl
from openpyxl.styles import PatternFill

# Read original
wb_orig = openpyxl.load_workbook('Datas/common_beans.xlsx', read_only=True)
ws_orig = wb_orig['beans']

# Get all data
all_data = []
for row_idx in range(1, ws_orig.max_row + 1):
    row = []
    for col_idx in range(1, ws_orig.max_column + 1):
        cell = ws_orig.cell(row=row_idx, column=col_idx)
        row.append((cell.value, cell.font.bold if cell.font else False))
    all_data.append(row)

wb_orig.close()

# Create new workbook
wb = openpyxl.Workbook()
ws = wb.active
ws.title = 'beans'

# Write data with an extra column in row 1 after *fields
for row_idx, row_data in enumerate(all_data, 1):
    if row_idx == 1:
        # Find *fields position and add empty cell after
        for col_idx, (val, bold) in enumerate(row_data, 1):
            ws.cell(row=row_idx, column=col_idx, value=val)
            if val == '*fields':
                # Insert a new empty cell
                ws.cell(row=row_idx, column=col_idx + len(row_data) + 1, value=None)
                break
    else:
        for col_idx, (val, bold) in enumerate(row_data, 1):
            ws.cell(row=row_idx, column=col_idx, value=val)

wb.save('Datas/common_beans_new2.xlsx')
wb.close()
print('Created common_beans_new2.xlsx')
