BITS 64

section .rodata
global nexos_tls_ca_start
global nexos_tls_ca_end

nexos_tls_ca_start:
    incbin "ports/tls/trust/cacert.pem"
    db 0
nexos_tls_ca_end:
