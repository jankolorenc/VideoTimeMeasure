if (row < intervals){
    deduction = duration * 2.5;
    result = (deduction > 0.5) ? 0.5 : (Math.round(deduction * 10) / 10);
}
else{
    if (row == intervals){
        total = 0;
        for(i=0; i<intervals; i++){
            total += table.getValue(i,column);
        }
    }
}
