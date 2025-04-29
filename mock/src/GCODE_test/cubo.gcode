G21 ; Unidade em mm
G90 ; Modo absoluto

; Início do trabalho
G0 X0 Y0 Z0 A0 F1000 ; Posição inicial (topo do cubo)

; Começa a descida e escavação simulada
G1 Z-15 A0 F500
G1 X-20 Y0 A45
G1 X20 Y0 A90
G1 X0 Y-20 A135
G1 X0 Y20 A180
G1 X-20 Y0 A225
G1 X20 Y0 A270
G1 X0 Y-20 A315
G1 X0 Y20 A360

; Segunda camada (mais profunda)
G1 Z-20 A0
G1 X-18 Y0 A45
G1 X18 Y0 A90
G1 X0 Y-18 A135
G1 X0 Y18 A180
G1 X-18 Y0 A225
G1 X18 Y0 A270
G1 X0 Y-18 A315
G1 X0 Y18 A360

; Terceira camada
G1 Z-25 A0
G1 X-15 Y0 A45
G1 X15 Y0 A90
G1 X0 Y-15 A135
G1 X0 Y15 A180
G1 X-15 Y0 A225
G1 X15 Y0 A270
G1 X0 Y-15 A315
G1 X0 Y15 A360

; Quarta camada (próxima do centro)
G1 Z-30 A0
G1 X-10 Y0 A45
G1 X10 Y0 A90
G1 X0 Y-10 A135
G1 X0 Y10 A180
G1 X-10 Y0 A225
G1 X10 Y0 A270
G1 X0 Y-10 A315
G1 X0 Y10 A360

; Finaliza a esfera
G1 Z1 A0
G0 X0 Y0 A0

; Fim do programa
M30
