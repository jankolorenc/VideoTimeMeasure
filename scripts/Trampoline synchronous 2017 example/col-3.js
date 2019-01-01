if (row < intervals){
    deduction = duration * 2.5;
    result = (deduction > 1.0) ? 1.0 : (Math.round(deduction * 100) / 100);
}
else{
    if (row == intervals){
        total = 0;
        for(i=0; i<intervals; i++){
            total += table.getValue(i,column);
        }
    }
}
