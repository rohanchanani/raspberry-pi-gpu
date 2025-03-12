document.addEventListener('DOMContentLoaded', function() {
    // Define register mappings (expanded for completeness)
    const registerMap = {
        // General purpose registers
        'r0': 0, 'r1': 1, 'r2': 2, 'r3': 3, 'r4': 4, 'r5': 5, 'r6': 6, 'r7': 7,
        'r8': 8, 'r9': 9, 'r10': 10, 'r11': 11, 'r12': 12, 'r13': 13, 'r14': 14, 'r15': 15,
        'r16': 16, 'r17': 17, 'r18': 18, 'r19': 19, 'r20': 20, 'r21': 21, 'r22': 22, 'r23': 23,
        'r24': 24, 'r25': 25, 'r26': 26, 'r27': 27, 'r28': 28, 'r29': 29, 'r30': 30, 'r31': 31,
        
        // Special registers
        'uniform': 32,      // Uniform read
        'varying': 35,      // Varying read
        'element': 38,      // Element number
        'qpu_num': 41,      // QPU number
        'nop': 45,          // No operation
        'vpm': 48,          // VPM read/write
        'vpm_ld_busy': 49,  // VPM load busy
        'vpm_st_busy': 50,  // VPM store busy
        'mutex': 51,        // Mutex
        'sfu_recip': 52,    // SFU reciprocal
        'sfu_rsqrt': 53,    // SFU reciprocal square root
        'sfu_exp': 54,      // SFU exponent
        'sfu_log': 55,      // SFU logarithm
        'tmu0_s': 56,       // TMU0 s coordinate
        'tmu0_t': 57,       // TMU0 t coordinate
        'tmu0_r': 58,       // TMU0 r coordinate
        'tmu0_b': 59,       // TMU0 b coordinate
        'tmu1_s': 60,       // TMU1 s coordinate
        'tmu1_t': 61,       // TMU1 t coordinate
        'tmu1_r': 62,       // TMU1 r coordinate
        'tmu1_b': 63,       // TMU1 b coordinate
        'null': 39          // NULL register (write sink)
    };

    // Define ALU opcodes (expanded)
    const aluOpcodes = {
        // Integer ALU operations
        'nop': 0,
        'add': 12,      // Integer addition
        'sub': 13,      // Integer subtraction
        'shr': 14,      // Integer shift right
        'asr': 15,      // Integer arithmetic shift right
        'ror': 16,      // Integer rotate right
        'shl': 17,      // Integer shift left
        'min': 18,      // Integer minimum
        'max': 19,      // Integer maximum
        'and': 20,      // Bitwise AND
        'or': 21,       // Bitwise OR
        'xor': 22,      // Bitwise XOR
        'not': 23,      // Bitwise NOT
        'clz': 24,      // Count leading zeros
        'v8adds': 30,   // Vector 8-bit add saturated
        'v8subs': 31,   // Vector 8-bit subtract saturated
        
        // Floating-point ALU operations
        'fadd': 1,      // Floating-point addition
        'fsub': 2,      // Floating-point subtraction
        'fmin': 4,      // Floating-point minimum
        'fmax': 5,      // Floating-point maximum
        'fminabs': 6,   // Floating-point minimum absolute
        'fmaxabs': 7,   // Floating-point maximum absolute
        'ftoi': 8,      // Floating-point to integer
        'itof': 9,      // Integer to floating-point
        'fmul': 3,      // Floating-point multiply
        
        // Special operations
        'mov': 37,      // Move/copy
        'ldvpm': 38,    // Load VPM
        'stvpm': 39,    // Store VPM
        'thrend': 240,  // Thread end
        'thrsw': 241,   // Thread switch
        'ldtmu0': 242,  // Load TMU0
        'ldtmu1': 243   // Load TMU1
    };

    // Define pack modes
    const packModes = {
        'none': 0,      // No packing
        'p16a': 1,      // 16-bit float in high half of destination
        'p16b': 2,      // 16-bit float in low half of destination
        'p8abcd': 8,    // 8-bit in 4 bytes of destination
        'p8a': 9,       // 8-bit in byte 0
        'p8b': 10,      // 8-bit in byte 1
        'p8c': 11,      // 8-bit in byte 2
        'p8d': 12       // 8-bit in byte 3
    };

    // Define unpack modes
    const unpackModes = {
        'none': 0,      // No unpacking
        'u16a': 1,      // 16-bit float from high half
        'u16b': 2,      // 16-bit float from low half
        'u8abcd': 8,    // 8-bit values from 4 bytes
        'u8a': 9,       // 8-bit from byte 0
        'u8b': 10,      // 8-bit from byte 1
        'u8c': 11,      // 8-bit from byte 2
        'u8d': 12       // 8-bit from byte 3
    };

    // Define condition codes (expanded)
    const conditionCodes = {
        'always': 0,     // Always execute
        'zero': 1,       // Zero flag set
        'not-zero': 2,   // Zero flag clear
        'negative': 3,   // Negative flag set
        'positive': 4,   // Negative flag clear
        'true': 0,       // Same as always
        'false': 5,      // Never execute
        'overflow': 6,   // Overflow flag set
        'not-overflow': 7 // Overflow flag clear
    };

    // Get DOM elements
    const instructionTypeSelect = document.getElementById('instruction-type');
    const aluOptions = document.getElementById('alu-options');
    const branchOptions = document.getElementById('branch-options');
    const loadImmediateOptions = document.getElementById('load-immediate-options');
    const loadOptions = document.getElementById('load-options');
    const assembleBtn = document.getElementById('assemble-btn');
    const assemblyInput = document.getElementById('assembly-input');
    const word0Hex = document.getElementById('word0-hex');
    const word1Hex = document.getElementById('word1-hex');
    const fullHex = document.getElementById('full-hex');
    const instructionDetails = document.getElementById('instruction-details');
    const srcBRegSelect = document.getElementById('src-b-reg');
    const immediateValueContainer = document.getElementById('immediate-value-container');

    // Show options based on instruction type
    instructionTypeSelect.addEventListener('change', function() {
        const selectedType = this.value;
        
        // Hide all options first
        aluOptions.style.display = 'none';
        branchOptions.style.display = 'none';
        loadImmediateOptions.style.display = 'none';
        loadOptions.style.display = 'none';
        
        // Show selected options
        if (selectedType === 'alu') {
            aluOptions.style.display = 'block';
        } else if (selectedType === 'branch') {
            branchOptions.style.display = 'block';
        } else if (selectedType === 'load-immediate') {
            loadImmediateOptions.style.display = 'block';
        } else if (selectedType === 'load') {
            loadOptions.style.display = 'block';
        }
    });

    // Show/hide immediate value input based on source B selection
    srcBRegSelect.addEventListener('change', function() {
        if (this.value === 'immediate') {
            immediateValueContainer.style.display = 'block';
        } else {
            immediateValueContainer.style.display = 'none';
        }
    });

    // Assemble button click handler
    assembleBtn.addEventListener('click', function() {
        if (assemblyInput.value.trim()) {
            // If there's text in the assembly input, parse it
            parseAndEncodeAssembly(assemblyInput.value.trim());
        } else {
            // Otherwise, use the builder form
            encodeFromBuilder();
        }
    });

    // Parse and encode assembly instruction from text input
    function parseAndEncodeAssembly(assemblyText) {
        // Basic parsing of assembly instruction
        const parts = assemblyText.replace(/,/g, ' ').split(/\s+/);
        const opcode = parts[0].toLowerCase();
        
        if (opcode in aluOpcodes) {
            // ALU instruction
            let dstReg, srcAReg, srcBReg;
            
            // Handle special case for mov which has only 2 operands
            if (opcode === 'mov' || opcode === 'not' || opcode === 'clz') {
                dstReg = parts[1];
                srcAReg = parts[2];
                srcBReg = parts[2]; // For mov, source A and B are the same
            } else {
                dstReg = parts[1];
                srcAReg = parts[2];
                srcBReg = parts[3];
            }
            
            // Check if we have an immediate value
            let immediateValue = 0;
            let useImmediate = false;
            if (srcBReg && srcBReg.match(/^#?-?\d+$/)) {
                immediateValue = parseInt(srcBReg.replace('#', ''));
                useImmediate = true;
                srcBReg = 'immediate';
            }
            
            encodeAluInstruction(opcode, dstReg, srcAReg, srcBReg, immediateValue, useImmediate);
        } else if (opcode === 'b' || opcode === 'branch') {
            // Branch instruction
            const condition = parts[1] || 'always';
            const target = parseInt(parts[2] || '0');
            encodeBranchInstruction(condition, target);
        } else if (opcode === 'li' || opcode === 'load') {
            // Load immediate instruction
            const dstReg = parts[1];
            const immediateValue = parseInt(parts[2] || '0');
            encodeLoadImmediateInstruction(dstReg, immediateValue);
        } else if (opcode === 'thrend') {
            // Thread end instruction - special case
            encodeThreadEndInstruction();
        } else if (opcode === 'thrsw') {
            // Thread switch instruction - special case
            encodeThreadSwitchInstruction();
        } else if (opcode === 'ldtmu0' || opcode === 'ldtmu1') {
            // TMU load instructions
            const dstReg = parts[1] || 'r0';
            encodeTmuLoadInstruction(opcode, dstReg);
        } else {
            updateOutput(0, 0, 'Unknown instruction: ' + opcode);
        }
    }

    // Encode instruction from the builder form
    function encodeFromBuilder() {
        const instructionType = instructionTypeSelect.value;
        
        if (instructionType === 'alu') {
            const opcode = document.getElementById('alu-op').value;
            const dstReg = document.getElementById('dst-reg').value;
            
            // Handle special instructions that don't need source operands
            if (opcode === 'thrend') {
                encodeThreadEndInstruction();
                return;
            } else if (opcode === 'thrsw') {
                encodeThreadSwitchInstruction();
                return;
            }
            
            const srcAReg = document.getElementById('src-a-reg').value;
            const srcBReg = document.getElementById('src-b-reg').value;
            const useImmediate = srcBReg === 'immediate';
            const immediateValue = useImmediate ? parseInt(document.getElementById('immediate-value').value) : 0;
            
            encodeAluInstruction(opcode, dstReg, srcAReg, srcBReg, immediateValue, useImmediate);
        } else if (instructionType === 'branch') {
            const condition = document.getElementById('branch-cond').value;
            const target = parseInt(document.getElementById('branch-target').value);
            
            encodeBranchInstruction(condition, target);
        } else if (instructionType === 'load-immediate') {
            const dstReg = document.getElementById('li-dst').value;
            const immediateValue = parseInt(document.getElementById('li-value').value);
            
            encodeLoadImmediateInstruction(dstReg, immediateValue);
        } else if (instructionType === 'load') {
            const loadOp = document.getElementById('load-op').value;
            const dstReg = document.getElementById('load-dst').value;
            
            if (loadOp === 'ldvpm') {
                encodeVpmLoadInstruction(dstReg);
            } else if (loadOp === 'stvpm') {
                encodeVpmStoreInstruction(dstReg);
            } else if (loadOp === 'ldtmu0' || loadOp === 'ldtmu1') {
                encodeTmuLoadInstruction(loadOp, dstReg);
            } else if (loadOp === 'lduniform') {
                encodeUniformLoadInstruction(dstReg);
            } else if (loadOp === 'ldvary') {
                encodeVaryingLoadInstruction(dstReg);
            }
        }
    }

    // Encode ALU instruction
    function encodeAluInstruction(opcode, dstReg, srcAReg, srcBReg, immediateValue = 0, useImmediate = false) {
        // Get opcode value
        const opcodeValue = aluOpcodes[opcode] || 0;
        
        // Get register values
        const dstRegValue = registerMap[dstReg] || 0;
        const srcARegValue = registerMap[srcAReg] || 0;
        let srcBRegValue = registerMap[srcBReg] || 0;
        
        // Word 0 - ALU encoding for add instruction
        // Format: [unused:12][srcB:6][addOp:5][dstReg:6][unused:3]
        let word0 = 0;
        
        // For most instructions, we use Add ALU
        if (useImmediate) {
            // Small immediate encoding (bit 31 = 1)
            word0 = (1 << 31) | (immediateValue & 0x7FFF) << 16 | (opcodeValue & 0x1F) << 11 | (dstRegValue & 0x3F) << 5;
        } else {
            // Register encoding
            word0 = (srcBRegValue & 0x3F) << 26 | (opcodeValue & 0x1F) << 21 | (dstRegValue & 0x3F) << 15;
        }
        
        // Word 1 - Control bits and operand A
        // Format: [signal:4][pm:1][pack:8][cond:3][unpack:3][srcA:6][ws:1][sf:1][reserved:5]
        const condValue = 0; // Always condition
        let word1 = (condValue & 0x7) << 23 | (srcARegValue & 0x3F) << 17;
        
        // For mov, we use a specific encoding
        if (opcode === 'mov') {
            word0 = 0x158E7D80;
            word1 = 0x10020827;
            
            // If we have destination as vpm
            if (dstReg === 'vpm') {
                word0 = 0x159E7000;
                word1 = 0x10020C67;
            }
            
            // If we have source as uniform
            if (srcAReg === 'uniform') {
                word0 = 0x15827D80;
                word1 = 0x10020827;
            }
        }
        
        // Special case for thrend (thread end)
        if (opcode === 'thrend') {
            word0 = 0x159E7900;
            word1 = 0x100009E7;
        }
        
        // Special case for thrsw (thread switch)
        if (opcode === 'thrsw') {
            word0 = 0x159E7900;
            word1 = 0x500009E7;
        }
        
        // Update output with encoded instruction
        updateOutput(word0, word1, buildAluDetails(word0, word1, opcode, dstReg, srcAReg, srcBReg, immediateValue, useImmediate));
    }

    // Encode branch instruction
    function encodeBranchInstruction(condition, target) {
        // Branch encoding is complex, this is simplified
        const condValue = conditionCodes[condition] || 0;
        
        // Simple branch encoding - this is not complete
        const word0 = 0x15000000 | ((target >> 3) & 0xFFFFF);
        const word1 = 0xD0000000 | ((condValue & 0x7) << 23) | ((target & 0x7) << 20);
        
        const details = `
            <p><strong>Instruction:</strong> Branch</p>
            <p><strong>Condition:</strong> ${condition}</p>
            <p><strong>Target address:</strong> ${target}</p>
            <p><strong>Binary representation:</strong></p>
            <p>Word 0: ${formatBinary(word0)}</p>
            <p>Word 1: ${formatBinary(word1)}</p>
        `;
        
        updateOutput(word0, word1, details);
    }

    // Encode load immediate instruction
    function encodeLoadImmediateInstruction(dstReg, immediateValue) {
        const dstRegValue = registerMap[dstReg] || 0;
        
        // Load immediate encoding - simplified
        const word0 = 0x14000000 | (dstRegValue & 0x3F) << 23 | (immediateValue & 0x7FFFFF);
        const word1 = 0xD0000000;
        
        const details = `
            <p><strong>Instruction:</strong> Load Immediate</p>
            <p><strong>Destination register:</strong> ${dstReg}</p>
            <p><strong>Immediate value:</strong> ${immediateValue}</p>
            <p><strong>Binary representation:</strong></p>
            <p>Word 0: ${formatBinary(word0)}</p>
            <p>Word 1: ${formatBinary(word1)}</p>
        `;
        
        updateOutput(word0, word1, details);
    }

    // Encode thread end instruction
    function encodeThreadEndInstruction() {
        // Special encoding for thread end
        const word0 = 0x159E7900;
        const word1 = 0x100009E7;
        
        const details = `
            <p><strong>Instruction:</strong> Thread End (thrend)</p>
            <p><strong>Binary representation:</strong></p>
            <p>Word 0: ${formatBinary(word0)}</p>
            <p>Word 1: ${formatBinary(word1)}</p>
        `;
        
        updateOutput(word0, word1, details);
    }

    // Encode thread switch instruction
    function encodeThreadSwitchInstruction() {
        // Special encoding for thread switch
        const word0 = 0x159E7900;
        const word1 = 0x500009E7;
        
        const details = `
            <p><strong>Instruction:</strong> Thread Switch (thrsw)</p>
            <p><strong>Binary representation:</strong></p>
            <p>Word 0: ${formatBinary(word0)}</p>
            <p>Word 1: ${formatBinary(word1)}</p>
        `;
        
        updateOutput(word0, word1, details);
    }

    // Encode TMU load instruction
    function encodeTmuLoadInstruction(opcode, dstReg) {
        const dstRegValue = registerMap[dstReg] || 0;
        
        // TMU load encoding
        let word0, word1;
        
        if (opcode === 'ldtmu0') {
            word0 = 0x159E7900;
            word1 = 0x30020BA7;
        } else { // ldtmu1
            word0 = 0x159E7900;
            word1 = 0x40020BA7;
        }
        
        // Set destination register
        word0 = (word0 & ~(0x3F << 6)) | (dstRegValue & 0x3F) << 6;
        
        const details = `
            <p><strong>Instruction:</strong> ${opcode}</p>
            <p><strong>Destination register:</strong> ${dstReg}</p>
            <p><strong>Binary representation:</strong></p>
            <p>Word 0: ${formatBinary(word0)}</p>
            <p>Word 1: ${formatBinary(word1)}</p>
        `;
        
        updateOutput(word0, word1, details);
    }

    // Add function to encode VPM load instruction
    function encodeVpmLoadInstruction(dstReg) {
        const dstRegValue = registerMap[dstReg] || 0;
        
        // VPM load encoding
        const word0 = 0x159E7480;
        const word1 = 0x10020827;
        
        // Set destination register
        const modifiedWord0 = (word0 & ~(0x3F << 6)) | (dstRegValue & 0x3F) << 6;
        
        const details = `
            <p><strong>Instruction:</strong> ldvpm</p>
            <p><strong>Destination register:</strong> ${dstReg}</p>
            <p><strong>Binary representation:</strong></p>
            <p>Word 0: ${formatBinary(modifiedWord0)}</p>
            <p>Word 1: ${formatBinary(word1)}</p>
        `;
        
        updateOutput(modifiedWord0, word1, details);
    }

    // Add function to encode VPM store instruction
    function encodeVpmStoreInstruction(srcReg) {
        const srcRegValue = registerMap[srcReg] || 0;
        
        // VPM store encoding
        const word0 = 0x159E7000;
        const word1 = 0x10020C67;
        
        // Set source register (which is actually in the ALU input)
        const modifiedWord1 = (word1 & ~(0x3F << 17)) | (srcRegValue & 0x3F) << 17;
        
        const details = `
            <p><strong>Instruction:</strong> stvpm</p>
            <p><strong>Source register:</strong> ${srcReg}</p>
            <p><strong>Binary representation:</strong></p>
            <p>Word 0: ${formatBinary(word0)}</p>
            <p>Word 1: ${formatBinary(modifiedWord1)}</p>
        `;
        
        updateOutput(word0, modifiedWord1, details);
    }

    // Add function to encode uniform load instruction
    function encodeUniformLoadInstruction(dstReg) {
        const dstRegValue = registerMap[dstReg] || 0;
        
        // Uniform load encoding (this is a move from uniform to the destination)
        const word0 = 0x15827D80;
        const word1 = 0x10020827;
        
        // Set destination register
        const modifiedWord0 = (word0 & ~(0x3F << 6)) | (dstRegValue & 0x3F) << 6;
        
        const details = `
            <p><strong>Instruction:</strong> lduniform</p>
            <p><strong>Destination register:</strong> ${dstReg}</p>
            <p><strong>Binary representation:</strong></p>
            <p>Word 0: ${formatBinary(modifiedWord0)}</p>
            <p>Word 1: ${formatBinary(word1)}</p>
        `;
        
        updateOutput(modifiedWord0, word1, details);
    }

    // Add function to encode varying load instruction
    function encodeVaryingLoadInstruction(dstReg) {
        const dstRegValue = registerMap[dstReg] || 0;
        
        // Varying load encoding (this is a move from varying to the destination)
        const word0 = 0x15867D80;
        const word1 = 0x10020827;
        
        // Set destination register
        const modifiedWord0 = (word0 & ~(0x3F << 6)) | (dstRegValue & 0x3F) << 6;
        
        const details = `
            <p><strong>Instruction:</strong> ldvary</p>
            <p><strong>Destination register:</strong> ${dstReg}</p>
            <p><strong>Binary representation:</strong></p>
            <p>Word 0: ${formatBinary(modifiedWord0)}</p>
            <p>Word 1: ${formatBinary(word1)}</p>
        `;
        
        updateOutput(modifiedWord0, word1, details);
    }

    // Update output display
    function updateOutput(word0, word1, details) {
        // Format as hex
        word0Hex.textContent = '0x' + word0.toString(16).padStart(8, '0').toUpperCase();
        word1Hex.textContent = '0x' + word1.toString(16).padStart(8, '0').toUpperCase();
        fullHex.textContent = `0x${word0.toString(16).padStart(8, '0').toUpperCase()}, 0x${word1.toString(16).padStart(8, '0').toUpperCase()}`;
        
        // Display instruction details
        instructionDetails.innerHTML = details;
    }

    // Format a 32-bit number as a binary string with spaces every 4 bits
    function formatBinary(num) {
        let binary = '';
        for (let i = 31; i >= 0; i--) {
            binary += (num >> i) & 1;
            if (i % 4 === 0 && i > 0) binary += ' ';
        }
        return binary;
    }

    // Build ALU instruction details
    function buildAluDetails(word0, word1, opcode, dstReg, srcAReg, srcBReg, immediateValue, useImmediate) {
        let details = '';
        
        details += `<p><strong>Instruction:</strong> ${opcode}</p>`;
        details += `<p><strong>Destination:</strong> ${dstReg}</p>`;
        details += `<p><strong>Source A:</strong> ${srcAReg}</p>`;
        
        if (useImmediate) {
            details += `<p><strong>Source B:</strong> Immediate value ${immediateValue}</p>`;
        } else {
            details += `<p><strong>Source B:</strong> ${srcBReg}</p>`;
        }
        
        details += '<p><strong>Binary representation:</strong></p>';
        details += `<p>Word 0: ${formatBinary(word0)}</p>`;
        details += `<p>Word 1: ${formatBinary(word1)}</p>`;
        
        return details;
    }

    // Initialize by showing ALU options by default
    aluOptions.style.display = 'block';
    
    // Update the HTML for more comprehensive instructions
    // These could be dynamically generated, but for simplicity we'll update them here
    const instructionReferenceBody = document.querySelector('#instruction-reference tbody');
    
    // Clear existing rows
    instructionReferenceBody.innerHTML = '';
    
    // Add comprehensive instruction set
    const referenceInstructions = [
        { mnemonic: 'nop', opcode: '0x00', description: 'No operation' },
        { mnemonic: 'fadd', opcode: '0x01', description: 'Floating-point addition' },
        { mnemonic: 'fsub', opcode: '0x02', description: 'Floating-point subtraction' },
        { mnemonic: 'fmul', opcode: '0x03', description: 'Floating-point multiplication' },
        { mnemonic: 'fmin', opcode: '0x04', description: 'Floating-point minimum' },
        { mnemonic: 'fmax', opcode: '0x05', description: 'Floating-point maximum' },
        { mnemonic: 'fminabs', opcode: '0x06', description: 'Floating-point minimum absolute' },
        { mnemonic: 'fmaxabs', opcode: '0x07', description: 'Floating-point maximum absolute' },
        { mnemonic: 'ftoi', opcode: '0x08', description: 'Floating-point to integer conversion' },
        { mnemonic: 'itof', opcode: '0x09', description: 'Integer to floating-point conversion' },
        { mnemonic: 'add', opcode: '0x0C', description: 'Integer addition' },
        { mnemonic: 'sub', opcode: '0x0D', description: 'Integer subtraction' },
        { mnemonic: 'shr', opcode: '0x0E', description: 'Logical shift right' },
        { mnemonic: 'asr', opcode: '0x0F', description: 'Arithmetic shift right' },
        { mnemonic: 'ror', opcode: '0x10', description: 'Rotate right' },
        { mnemonic: 'shl', opcode: '0x11', description: 'Shift left' },
        { mnemonic: 'min', opcode: '0x12', description: 'Integer minimum' },
        { mnemonic: 'max', opcode: '0x13', description: 'Integer maximum' },
        { mnemonic: 'and', opcode: '0x14', description: 'Bitwise AND' },
        { mnemonic: 'or', opcode: '0x15', description: 'Bitwise OR' },
        { mnemonic: 'xor', opcode: '0x16', description: 'Bitwise XOR' },
        { mnemonic: 'not', opcode: '0x17', description: 'Bitwise NOT' },
        { mnemonic: 'clz', opcode: '0x18', description: 'Count leading zeros' },
        { mnemonic: 'v8adds', opcode: '0x1E', description: 'Vector 8-bit addition (saturated)' },
        { mnemonic: 'v8subs', opcode: '0x1F', description: 'Vector 8-bit subtraction (saturated)' },
        { mnemonic: 'mov', opcode: '0x25', description: 'Move register' },
        { mnemonic: 'ldvpm', opcode: '0x26', description: 'Load from VPM' },
        { mnemonic: 'stvpm', opcode: '0x27', description: 'Store to VPM' },
        { mnemonic: 'thrend', opcode: '0xF0', description: 'Thread end' },
        { mnemonic: 'thrsw', opcode: '0xF1', description: 'Thread switch' },
        { mnemonic: 'ldtmu0', opcode: '0xF2', description: 'Load from TMU0' },
        { mnemonic: 'ldtmu1', opcode: '0xF3', description: 'Load from TMU1' }
    ];
    
    referenceInstructions.forEach(instruction => {
        const row = document.createElement('tr');
        
        const mnemonicCell = document.createElement('td');
        mnemonicCell.textContent = instruction.mnemonic;
        
        const opcodeCell = document.createElement('td');
        opcodeCell.textContent = instruction.opcode;
        
        const descriptionCell = document.createElement('td');
        descriptionCell.textContent = instruction.description;
        
        row.appendChild(mnemonicCell);
        row.appendChild(opcodeCell);
        row.appendChild(descriptionCell);
        
        instructionReferenceBody.appendChild(row);
    });

    // Add disassembler button handler
    document.getElementById('disassemble-btn').addEventListener('click', function() {
        const input = document.getElementById('disassembly-input').value.trim();
        if (input) {
            const disassembledCode = disassembleCode(input);
            document.getElementById('disassembly-output').textContent = disassembledCode;
        }
    });

    // Main disassembler function
    function disassembleCode(hexCode) {
        // Split input into individual hex values
        const hexValues = hexCode.split(/\s+/).filter(x => x);
        let output = '';
        
        // Process two values at a time
        for (let i = 0; i < hexValues.length; i += 2) {
            if (i + 1 >= hexValues.length) {
                output += `Error: Missing second word for instruction at ${hexValues[i]}\n`;
                break;
            }
            
            const word0Hex = hexValues[i];
            const word1Hex = hexValues[i + 1];
            
            // Validate hex format
            if (!/^0x[0-9a-fA-F]{8}$/.test(word0Hex) || !/^0x[0-9a-fA-F]{8}$/.test(word1Hex)) {
                output += `Error: Invalid hex format at ${word0Hex} ${word1Hex}\n`;
                continue;
            }
            
            // Convert to numbers
            const word0 = parseInt(word0Hex, 16);
            const word1 = parseInt(word1Hex, 16);
            
            // Disassemble the instruction pair
            const instruction = disassembleInstruction(word0, word1);
            output += `${word0Hex} ${word1Hex}  ${instruction}\n`;
        }
        
        return output;
    }

    // Disassemble a single instruction
    function disassembleInstruction(word0, word1) {
        // Extract common fields
        const opcode = (word0 >> 24) & 0xFF;
        const dstReg = (word0 >> 6) & 0x3F;
        const srcAReg = (word0 >> 12) & 0x3F;
        const srcBReg = (word1 >> 17) & 0x3F;
        const condition = (word1 >> 23) & 0x7;
        const isImmediate = (word1 & 0x10000000) !== 0;
        const immediateValue = word1 & 0xFFFF;
        
        // Get register names
        const dstRegName = getRegisterName(dstReg);
        const srcARegName = getRegisterName(srcAReg);
        const srcBRegName = getRegisterName(srcBReg);
        
        // Get condition name
        const conditionName = getConditionName(condition);
        
        // Lookup opcode
        const opcodeInfo = Object.entries(aluOpcodes).find(([_, code]) => code === opcode);
        const mnemonic = opcodeInfo ? opcodeInfo[0] : 'unknown';
        
        // Handle special instructions
        if (mnemonic === 'thrend') {
            return 'thrend';
        }
        if (mnemonic === 'thrsw') {
            return 'thrsw';
        }
        
        // Handle load/store instructions
        if (mnemonic === 'ldvpm') {
            return `ldvpm ${dstRegName}`;
        }
        if (mnemonic === 'stvpm') {
            return `stvpm ${srcARegName}`;
        }
        
        // Handle ALU instructions
        if (mnemonic in aluOpcodes) {
            if (isImmediate) {
                return `${mnemonic} ${dstRegName}, ${srcARegName}, #${immediateValue}`;
            } else {
                return `${mnemonic} ${dstRegName}, ${srcARegName}, ${srcBRegName}`;
            }
        }
        
        return `unknown (opcode: 0x${opcode.toString(16)})`;
    }

    // Get register name from register number
    function getRegisterName(regNum) {
        const entry = Object.entries(registerMap).find(([_, num]) => num === regNum);
        return entry ? entry[0] : `r${regNum}`;
    }

    // Get condition name from condition code
    function getConditionName(condCode) {
        const entry = Object.entries(conditionCodes).find(([_, code]) => code === condCode);
        return entry ? entry[0] : 'unknown';
    }
});
