function [duty_cycle_PS_A, duty_cycle_PS_B,V_ac_rif, V_ac_rif_lim]=Controllo_Corrente_Bobina_Remota_e_Limitazione_Corrente_Bobina_Locale(V_dc, I_coil_rem_err, I_coil_loc)
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                                    %
%          Controllo dell'ampiezza della corrente nella bobina ricevente             %
%          mediante tensione generata dal convertitore dc-ac trasmittente            %
%              Limitazione della corrente nella bobina trasmittente                  %
%                                                                                    %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
    % Per ottenere i duty-cycle per il convertitore dc-ac è necessario
    % calcolare la funzione
    % 1/pi*arcoseno(V_ac_rif/V_ac_rif_max).
    % A questo scopo è utilizzata una tabella per effettuare una interpolazione lineare.
    %
    % Tabella usata per l'interpolazione della funzione 1/pi*arcoseno() tra 0 e 1
    % La tabella contiene 101 elementi. L'ultimo è usato solo per semplificare il codice.
    % L'indice della tabella in C varia tra 0 e 100. Corrisponde all'argomento dell'arcoseno moltiplicato per 100. 


    tabella_arcsin=[0.000000000000000,   0.003183151915873,   0.006366622213270,   0.009550729560432,   0.012735793199755,   0.015922133236660, ...
            0.019110070930640,   0.022299928989202,   0.025492031865477,   0.028686706060258,   0.031884280429260,   0.035085086496430,...
            0.038289458774147,   0.041497735091205,   0.044710256929508,   0.047927369770437,   0.051149423451922,   0.054376772537300,...
            0.057609776697097,   0.060848801104951,   0.064094216848975,   0.067346401359940,   0.070605738857752,   0.073872620817828,...
            0.077147446459050,   0.080430623255166,   0.083722567471605,   0.087023704729853,   0.090334470601733,   0.093655311236095,...
            0.096986684020678,   0.100329058282131,   0.103682916027458,   0.107048752730465,   0.110427078167105,   0.113818417304015,...
            0.117223311244961,   0.120642318240358,   0.124076014765585,   0.127524996674404,   0.130989880434455,   0.134471304452546,...
            0.137969930498342,   0.141486445235958,   0.145021561874111,   0.148576021946683,   0.152150597236966,   0.155746091860452,...
            0.159363344522883,   0.163003230972354,   0.166666666666667,   0.170354609679922,   0.174068063875524,   0.177808082376468,...
            0.181575771368100,   0.185372294273510,   0.189198876347599,   0.193056809742680,   0.196947459106530,   0.200872267783327,...
            0.204832764699133,   0.208830572026983,   0.212867413742587,   0.216945125200853,   0.221065663886429,   0.225231121519470,...
            0.229443737731699,   0.233705915569418,   0.238020239131091,   0.242389493710302,   0.246816688893365,   0.251305085159287,...
            0.255858224653840,   0.260479966967229,   0.265174530946820,   0.269946543837384,   0.274801099381575,   0.279743826961257,...
            0.284780974456357,   0.289919508299921,   0.295167235300867,   0.300532952314905,   0.306026631958643,   0.311659655572965,...
            0.317445109006172,   0.323398163238602,   0.329536571616801,   0.335881330554762,   0.342457574575956,   0.349295816015145,...
            0.356433706871294,   0.363918619603224,   0.371811566302050,   0.380193416581985,   0.389175313395541,   0.398917375895740,...
            0.409665529398267,   0.421834068069044,   0.436231439141480,   0.454946586355588,   0.500000000000000,   0.454946586355588];

    % Controllore dell'ampiezza della corrente nella bobina remota 
    % Banda Passante 85 Hz. Ts=1/21.25 kHz
    K_I_coil_err=0.002436907237795;      % Guadagno applicato all'errore di corrente attuale nella bobina remota
    K_I_coil_err_p=0.004873814475590;    % Guadagno applicato all'errore di corrente di un passo precedente nella bobina remota
    K_I_coil_err_pp=0.002436907237795;   % Guadagno applicato all'errore di corrente di due passi precedenti nella bobina remota
    
    K_V_coil_rif_p=1.901603650787137;    % Guadagno applicato al riferimento di tensione del passo precedente
    K_V_coil_rif_pp=-0.901603650787137;  % Guadagno applicato al riferimento di tensione di due passi precedenti

    I_coil_loc_lim=50;      % Ampiezza massima della corrente nella bobina trasmittente

    % Limitazione dell'ampiezza della corrente nella bobina trasmittente
    % Banda Passante 500 Hz. Ts=1/21.25 kHz      80°
    K_I_coil_loc_err=0.436953635294118;       % Guadagno applicato all'errore attuale di corrente nella bobina locale
    K_I_coil_loc_err_p=-0.406566364705882;    % Guadagno applicato all'errore di corrente di un passo precedente nella bobina locale

    K_V_coil_lim_p=1;    % Guadagno applicato al limite di tensione del passo precedente

    persistent Controllo_Corrente_Bobina_Remota_inizializzato
    persistent I_coil_rem_err_p
    persistent I_coil_rem_err_pp % Errore di corrente nella bobina all'istante di campionamento precedente
    persistent V_ac_rif_p
    persistent V_ac_rif_pp % Riferimenti di tensione da applicare alla bobina all'istante precedente

    persistent I_coil_loc_err_p
    persistent V_ac_rif_lim_p
    

    if(isempty(Controllo_Corrente_Bobina_Remota_inizializzato))
        Controllo_Corrente_Bobina_Remota_inizializzato=1;
        I_coil_rem_err_p=0;
        I_coil_rem_err_pp=0;
        V_ac_rif_p=0;
        V_ac_rif_pp=0;
        I_coil_loc_err_p=0;
        V_ac_rif_lim_p=0;
    end

    % Massima tensione ac che può essere generata
    V_ac_rif_max = V_dc*4/pi;
    
    % Riferimento di tensione
    V_ac_rif = K_V_coil_rif_p*V_ac_rif_p + K_V_coil_rif_pp*V_ac_rif_pp + K_I_coil_err*I_coil_rem_err + K_I_coil_err_p*I_coil_rem_err_p + K_I_coil_err_pp*I_coil_rem_err_pp;

    % Limitazione del riferimento di tensione
    if(V_ac_rif>V_ac_rif_max) % La prima armonica di tensione generata dal convertitore dc-ac non può essere maggiore di 4/pi * Vdc
        V_ac_rif=V_ac_rif_max;
    end 
    
    if(V_ac_rif<0)
        V_ac_rif=0;
    end 

    % Calcolo del limite riferimento di tensione
    I_coil_loc_err=I_coil_loc_lim-I_coil_loc;
    % Riferimento limite di tensione
    V_ac_rif_lim = K_V_coil_lim_p*V_ac_rif_lim_p + K_I_coil_loc_err*I_coil_loc_err + K_I_coil_loc_err_p*I_coil_loc_err_p;


    % Limitazione del riferimento di tensione
    if(V_ac_rif_lim>V_ac_rif_max) % La prima armonica di tensione generata dal convertitore dc-ac non può essere maggiore di 4/pi * Vdc
        V_ac_rif_lim=V_ac_rif_max;
    end 
    
    if(V_ac_rif_lim<0)
        V_ac_rif_lim=0;
    end
 
    if(V_ac_rif>V_ac_rif_lim)
        V_ac_rif=V_ac_rif_lim;
    end    

    if(V_ac_rif_lim>(V_ac_rif+1))
        V_ac_rif_lim=V_ac_rif+1;
    end    


    %V_ac_rif=V_ac_rif_lim;

    arg_arcsin=V_ac_rif/V_ac_rif_max; % argomento della funzione arcoseno
    
    if(arg_arcsin>1)
        arg_arcsin=1;
    end    
    
    % interpolazione della funzione arcoseno
    indice_argomento=arg_arcsin*100; % indice ella tabella 
    indice_argomento_prec=floor(indice_argomento); % corrisponde a (int)(indice argomento) E' l'intero immediatamente inferiore all'argomento dell'arcoseno*100. 
    indice_argomento_suc=indice_argomento_prec+1; % Intero immediatamente superiore all'argomento dell'arcoseno*100.
    % il valore "corretto" dell'arcoseno di arg_arcsin è compreso tra i due
    % elementi della tabela che si trovano nelle posizioni indice_argomento_prec e indice_argomento_suc
    duty_prec=tabella_arcsin(indice_argomento_prec+1); 
    duty_suc=tabella_arcsin(indice_argomento_suc+1);
    
    % Approssimo il valore corretto mediante una interpolazione lineare
    delta_indice_argomento=indice_argomento-indice_argomento_prec; 
    duty=duty_prec+delta_indice_argomento*(duty_suc-duty_prec);
    
    duty_cycle_PS_A=0.5+duty;
    duty_cycle_PS_B=0.5-duty;

    V_ac_rif_pp=V_ac_rif_p;
    V_ac_rif_p=V_ac_rif;
    I_coil_rem_err_pp=I_coil_rem_err_p;
    I_coil_rem_err_p=I_coil_rem_err;


    V_ac_rif_lim_p=V_ac_rif_lim;
    I_coil_loc_err_p=I_coil_loc_err;
end